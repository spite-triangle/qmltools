#include "qmlLanguageModel.h"

#include <stdlib.h>

#include <QFile>
#include <QFileInfo>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsreformatter.h>

#include "utils/qrcparser.h"
#include "utils/mimeutils.h"
#include "utils/algorithm.h"
#include "utils/mimeconstants.h"

#include "common/utils.h"
#include "common/lspLog.hpp"
#include "common/lspProject.h"
#include "common/jsonUtil.hpp"
#include "common/jsonSerializer.hpp"

#include "qmlModel/qmljsOpenedFileManager.h"

using namespace QmlJS;

namespace Private
{

bool HasRelocatableFlag(const QVersionNumber & version){
    return (version > QVersionNumber(4, 8, 4) && version < QVersionNumber(5, 0, 0)) 
            || version >= QVersionNumber(5, 1, 0);
}

double ParseHexChannel(QString hex, bool & ok){
    int hexnum = hex.toInt(&ok,16);
    if(ok == false) return 0.;
    return hexnum / 255.0f;
}

} // namespace Internal

QmlLanguageModel::QmlLanguageModel(QObject *parent)
    : QObject(parent)
{
    auto modelManager = new ModelManager();

    connect(modelManager, &QmlJS::ModelManagerInterface::documentUpdated, this, &QmlLanguageModel::onDocumentUpdated);
    connect(modelManager, &QmlJS::ModelManagerInterface::projectInfoUpdated, this, &QmlLanguageModel::onProjectInfoUpdated);

    m_bValid.store(false);
}

QmlLanguageModel::ProjectInfo QmlLanguageModel::creatProjectInfo()
{
    using namespace ProjectExplorer;
    ProjectInfo projectInfo;

    auto project = Project::Instance();
    ASSERT_RETURN(project != nullptr, "project == nullptr", projectInfo);
    projectInfo.project = project;

    /* qmlplugindump.exe */
    auto qmlDumpPath = Utils::FilePath::fromString(project->sdkAssetPath(Project::SDK_TOOL_PLUGINDUMP));
    if(qmlDumpPath.exists() == true){
        projectInfo.qmlDumpEnvironment = Utils::Environment::systemEnvironment();
        projectInfo.tryQmlDump = true;
        projectInfo.qmlDumpPath = qmlDumpPath;

        for (auto & path : project->getQml2ImportPath())
        {
            projectInfo.qmlDumpEnvironment.appendOrSet("QML2_IMPORT_PATH", path);
        }
    }

    /* 版本 */
    auto version = project->getVersionSdk();
    projectInfo.qmlDumpHasRelocatableFlag = Private::HasRelocatableFlag(version);
    projectInfo.qtVersionString = version.toString();

    /* 目标路径 */
    auto root = project->getProjectFolder();
    for(auto & path : project->getTargetFolder()){
        if(path.startsWith(root) == false) continue;
        projectInfo.applicationDirectories.append(Utils::FilePath::fromString(path));
    }

    projectInfo.qtQmlPath = Utils::FilePath::fromString(project->sdkAssetPath(Project::SDK_FOLDER_QML));
    
    for (auto & path : project->getImportPaths())
    {
        projectInfo.importPaths.maybeInsert(Utils::FilePath::fromString(path), QmlJS::Dialect::Qml);
    }
    projectInfo.importPaths.maybeInsert(projectInfo.qtQmlPath, QmlJS::Dialect::Qml);
    
    for(auto & path : project->getQrcFiles()){
        projectInfo.allResourceFiles.append(Utils::FilePath::fromString(path));
    }

    /* bundle */
    const QHash<QString, QString> replacements = {{QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath.toString()}};
    for (IBundleProvider *bp : IBundleProvider::allBundleProviders()){
        bp->mergeBundlesForKit(projectInfo.activeBundle, replacements);
    }
    projectInfo.extendedBundle = projectInfo.activeBundle;

    /* 查找原始资源 */
    if(project->sizeSourceFile() <= 0){
        auto sourceFolders = project->getSourceFolder();
        if(sourceFolders.size() <= 0){
            projectInfo.sourceFiles = querySources(project->getProjectFolder());
        }else{
            projectInfo.sourceFiles = querySources(sourceFolders);
        }

        projectInfo.sourceFiles = Utils::filteredUnique(projectInfo.sourceFiles);
        project->appendSourceFile(projectInfo.sourceFiles);
    }else{
        for (auto & file : project->getSourceFiles())
        {
            projectInfo.sourceFiles.append(file);
        }
    }

    projectInfo.allResourceFiles = Utils::filteredUnique(projectInfo.allResourceFiles);
    projectInfo.applicationDirectories = Utils::filteredUnique(projectInfo.applicationDirectories);

    return projectInfo;
}

bool QmlLanguageModel::restProjectInfo()
{
    // model 在更新 ProjectInfo , model 无效
    m_bValid.store(false);

    auto project = ProjectExplorer::Project::Instance();
    auto modelManager = ModelManagerInterface::instance();

    auto projectInfo = creatProjectInfo();

    /* 更新完毕后触发 ModelManagerInterface::projectInfoUpdated */
    modelManager->updateProjectInfo(projectInfo, project.get());

    // // projectInfo 更新后，再重新刷新
    std::lock_guard<std::mutex> lock(m_muteModelUpdate); 
    if(m_futureModelUpdate.isValid()){
        m_futureModelUpdate.waitForFinished();
    }

    m_futureModelUpdate = QtConcurrent::run([](){
        auto mm = ModelManagerInterface::instance();
        mm->test_joinAllThreads();

        auto model = QmlLanguageModel::Instance();
        model->setValid(true);
    });
    return true;
}

void QmlLanguageModel::onProjectInfoUpdated(const ProjectInfo &project) {
    // 对所有的 qrc 文件进行检测
    for(auto & file : project.allResourceFiles){
        auto parse = Utils::QrcParser::parseQrcFile(file.toString(), QString());
        auto json = diagnosticMsgToJson(file.toString(), parse->errorMessages());
        emit sigDiagnosticMessageUpdated(json);
    }
}

bool QmlLanguageModel::updateSourceFile()
{
    auto modelManager = ModelManagerInterface::instance();
    auto project = ProjectExplorer::Project::Instance();
    ASSERT_RETURN(modelManager != nullptr && project != nullptr, "modelManager == nullptr || project == nullptr" ,false);
    
    Utils::FilePaths paths;
    for (auto & path : project->getSourceFiles())
    {
        paths.append(path);
    }
    
    modelManager->updateSourceFiles(paths, true);
    return true;
}

bool QmlLanguageModel::updateSourceFile(const QString &strFile)
{
    auto modelManager = ModelManagerInterface::instance();
    auto project = ProjectExplorer::Project::Instance();
    ASSERT_RETURN(modelManager != nullptr && project != nullptr, "modelManager == nullptr || project == nullptr" ,false);

    // qrc 文件
    if(strFile.endsWith(".qrc") == true){
        auto parse = Utils::QrcParser::parseQrcFile(strFile, QString());
        auto json = diagnosticMsgToJson(strFile, parse->errorMessages());
        emit sigDiagnosticMessageUpdated(json);

        if(parse->isValid() == false) return false;
    }

    // qmldir 文件
    if(strFile.endsWith("/qmldir")){
        resetModle();
        return true;
    }

    modelManager->updateSourceFiles({Utils::FilePath::fromString(strFile)}, false);
    return true;
}

bool QmlLanguageModel::appendSourceFile(const QStringList &lstFile)
{
    auto modelManager = ModelManagerInterface::instance();
    auto project = ProjectExplorer::Project::Instance();
    ASSERT_RETURN(modelManager != nullptr && project != nullptr, "modelManager == nullptr || project == nullptr" ,false);

    Utils::FilePaths paths;
    for (auto & strFile : lstFile)
    {
        // qrc 文件
        if(strFile.endsWith(".qrc") == true){
            auto parse = Utils::QrcParser::parseQrcFile(strFile, QString());
            auto json = diagnosticMsgToJson(strFile, parse->errorMessages());
            emit sigDiagnosticMessageUpdated(json);

            if(parse->isValid() == false) return false;
        }

        paths.append(Utils::FilePath::fromString(strFile));
    }

    project->appendSourceFile(paths);
    modelManager->updateSourceFiles(paths, false);
    return true;
}

void QmlLanguageModel::resetModle() {
    auto mm = ModelManagerInterface::instance();

    setValid(false);
    mm->resetCodeModel();

    std::lock_guard<std::mutex> lock(m_muteModelUpdate); 
    if(m_futureModelUpdate.isValid()){
        m_futureModelUpdate.waitForFinished();
    }

    m_futureModelUpdate = QtConcurrent::run([=](){
        auto mm = ModelManagerInterface::instance();
        mm->test_joinAllThreads();

        auto model = QmlLanguageModel::Instance();
        model->setValid(true);

        model->setCurrFocusFile(QString());
        auto projectInfo = mm->projectInfo(ProjectExplorer::Project::Instance());
        mm->updateSourceFiles(projectInfo.sourceFiles, false);
    });
}

void QmlLanguageModel::waitModleUpdate() {

    std::lock_guard<std::mutex> lock(m_muteModelUpdate); 
    if(m_futureModelUpdate.isValid()){
        m_futureModelUpdate.waitForFinished();
    }

    m_futureModelUpdate = QFuture<void>();
}

void QmlLanguageModel::waitModelManagerUpdate()
{
    auto mm = ModelManagerInterface::instance();
    mm->test_joinAllThreads();
}

bool QmlLanguageModel::isValid()
{
    return m_bValid;
}

void QmlLanguageModel::onDocumentUpdated(QmlJS::Document::Ptr doc) {
    JsonPtr diagnosticJson;
    auto strPath = doc->fileName().toString();

    // 等模型第一次初始化完成，才有效
    if(m_bValid == false) return;

    // NOTE - 当前正专注的文件与检测不文件不是同一个就跳过，不然会有很多 qml 文件
    auto currFocus = getCurrFocusFile();
    if(currFocus.isEmpty() == true){
        auto project = ProjectExplorer::Project::Instance();
        if(project->containSourceFile(strPath) == false) return;
    }else{
        if(currFocus != strPath) return;
    }

    // 语法正确，需要更新 m_currSemantic
    if (doc->ast()) {
        auto mm = ModelManagerInterface::instance();

        bool bRes = false;
        auto content = loadFile(strPath, bRes);
        if(bRes == false) return;

        QTextDocumentPtr qdoc(new QTextDocument(OpenedFileManager::Instance()->fileContent(currFocus)));
        QmlJS::SemanticInfo info = QmlJS::SemanticInfo::makeNewSemanticInfo(doc, mm->snapshot(), qdoc);
        setCurrentSemantic(info);
 
        diagnosticJson = diagnosticMsgToJson(strPath, info.staticAnalysisMessages, info.semanticMessages);
    }else{
        diagnosticJson = diagnosticMsgToJson(strPath,doc->diagnosticMessages());
    }

    emit sigDiagnosticMessageUpdated(diagnosticJson);
}



JsonPtr QmlLanguageModel::diagnosticMsgToJson(const QString & path, const QList<QmlJS::DiagnosticMessage> &lstMsg)
{
    auto jsonPtr = std::make_shared<Json>(JsonUtil::DiagnosticsMessage(filePathToUrl(path)));
    auto & lstDiagnostic = (*jsonPtr)["diagnostics"];

    for(auto & msg : lstMsg){
        if(msg.isValid() == false) continue;

        // 错误位置
        RANGE_S range;
        range.start.line = msg.loc.startLine - 1;
        range.start.character = msg.loc.startColumn - 1;
        range.end.line = range.start.line;
        range.end.character = msg.loc.length + msg.loc.startColumn;

        DIAGNOSTIC_SEVERITY_E enType = DIAGNOSTIC_SEVERITY_E::DS_INFORMATION;
        if(msg.isError() == true) enType = DIAGNOSTIC_SEVERITY_E::DS_ERROR;
        if(msg.isWarning() == true) enType = DIAGNOSTIC_SEVERITY_E::DS_WARNING;

        lstDiagnostic.push_back(
            JsonUtil::Diagnostics(range, enType, "qml", OwO::QStringToUtf8(msg.message))
        );
    } 
    return jsonPtr;
}

JsonPtr QmlLanguageModel::diagnosticMsgToJson(const QString & path, const QStringList &lstMsg)
{
    static QRegularExpression re(R"(line ([0-9]+), col ([0-9]+))");
    auto jsonPtr = std::make_shared<Json>(JsonUtil::DiagnosticsMessage(filePathToUrl(path)));
    auto & lstDiagnostic = (*jsonPtr)["diagnostics"];

    for(auto & msg : lstMsg){
        // XML error on line 7, col 0: Premature end of document.

        auto start = msg.indexOf("line",0, Qt::CaseInsensitive); if(start < 0) continue;
        auto end = msg.indexOf(":", start, Qt::CaseInsensitive); if(end < 0) continue;
        QString strPos = msg.mid(start, end - start);

        QRegularExpressionMatch match = re.match(strPos);
        if(match.hasMatch() == false) continue;

        RANGE_S range;
        range.start.line = match.captured(1).toLongLong() - 1;
        range.start.character = match.captured(2).toLongLong() - 1;
        range.end.line = range.start.line;
        range.end.character = range.start.character + 1;

        lstDiagnostic.push_back(
            JsonUtil::Diagnostics(range, DIAGNOSTIC_SEVERITY_E::DS_ERROR, "qml", OwO::QStringToUtf8(msg.mid(end + 1)))
        );

    } 
    return jsonPtr;
}

JsonPtr QmlLanguageModel::diagnosticMsgToJson(const QString &path, const QList<QmlJS::StaticAnalysis::Message> & lstAnalysisMsg, const QList<QmlJS::DiagnosticMessage> & lstDiagnostMsg)
{
    auto jsonPtr = std::make_shared<Json>(JsonUtil::DiagnosticsMessage(filePathToUrl(path)));
    auto & lstDiagnostic = (*jsonPtr)["diagnostics"];

    for(auto & msg : lstDiagnostMsg){
        if(msg.isValid() == false) continue;

        // 错误位置
        RANGE_S range;
        range.start.line = msg.loc.startLine - 1;
        range.start.character = msg.loc.startColumn - 1;
        range.end.line = range.start.line;
        range.end.character = msg.loc.length + msg.loc.startColumn;

        DIAGNOSTIC_SEVERITY_E enType = DIAGNOSTIC_SEVERITY_E::DS_INFORMATION;
        if(msg.isError() == true) enType = DIAGNOSTIC_SEVERITY_E::DS_ERROR;
        if(msg.isWarning() == true) enType = DIAGNOSTIC_SEVERITY_E::DS_WARNING;

        lstDiagnostic.push_back(
            JsonUtil::Diagnostics(range, enType, "qml", OwO::QStringToUtf8(msg.message))
        );
    }

    for (auto & msg : lstAnalysisMsg)
    {
        if(msg.isValid() == false) continue;

        auto diagnost =  msg.toDiagnosticMessage();

        RANGE_S range;
        range.start.line = diagnost.loc.startLine - 1;
        range.start.character = diagnost.loc.startColumn - 1;
        range.end.line = range.start.line;
        range.end.character = diagnost.loc.length + diagnost.loc.startColumn;

        DIAGNOSTIC_SEVERITY_E enType = DIAGNOSTIC_SEVERITY_E::DS_INFORMATION;
        switch (msg.severity)
        {
        case QmlJS::Severity::Error: 
            enType = DIAGNOSTIC_SEVERITY_E::DS_ERROR; break;
        case QmlJS::Severity::ReadingTypeInfoWarning:
        case QmlJS::Severity::Warning: 
            enType = DIAGNOSTIC_SEVERITY_E::DS_WARNING; break;
        case QmlJS::Severity::Hint: 
            enType = DIAGNOSTIC_SEVERITY_E::DS_HINT; break;
        case QmlJS::Severity::MaybeError: 
        case QmlJS::Severity::MaybeWarning: 
            enType = DIAGNOSTIC_SEVERITY_E::DS_INFORMATION; break;
        default:
            break;
        }

        lstDiagnostic.push_back(
            JsonUtil::Diagnostics(range, enType, "qml", OwO::QStringToUtf8(diagnost.message))
        );
    }
    
    return jsonPtr;
}



QList<Utils::FilePath> QmlLanguageModel::querySources(const QString & strFolder)
{
    using namespace Utils;
    static const QSet<QString> qmlTypeNames = {Utils::Constants::QML_MIMETYPE ,
                                        Utils::Constants::QBS_MIMETYPE,
                                        Utils::Constants::QMLPROJECT_MIMETYPE,
                                        Utils::Constants::QMLTYPES_MIMETYPE,
                                        Utils::Constants::QMLUI_MIMETYPE,
                                        Utils::Constants::JS_MIMETYPE};

    QList<FilePath> res;

    auto root = FilePath::fromString(strFolder);
    if(root.isDir() == false) return res;

    // IterationPolicy(const FilePath &item, const FilePathInfo &info)
    Utils::FilePath::iterateDirectories({root},[&](const FilePath &item){
            auto name = Utils::mimeTypeForFile(item, MimeMatchMode::MatchExtension).name();
            if(qmlTypeNames.contains(name) == true){
                res.append(item);
            }
            return IterationPolicy::Continue;
        },
        FileFilter(QStringList(), QDir::AllEntries, QDirIterator::Subdirectories));

    return res;
}


QList<Utils::FilePath> QmlLanguageModel::querySources(const QStringList & lstFolder)
{
     QList<Utils::FilePath> res;
     for (auto & folder : lstFolder)
     {
        res.append(querySources(folder));
     }
     return res;
}

std::string QmlLanguageModel::filePathToUrl(const QString &strPath)
{
    QString url = "file:///" + QUrl::toPercentEncoding(strPath,"/.");
    return OwO::QStringToUtf8(url);
}

QByteArray QmlLanguageModel::loadFile(const QString &strPath, bool &bRes)
{
    bRes = true;
    QByteArray content;
    QFile file(strPath);

    ASSERT_RETURN(file.exists() == true, "file is not exist", bRes = false, content);

    // NOTE - Qfile 读取文件有概率会为空
    BLOCK_TRY(4,0){
        ASSERT_RETURN(file.open(QIODevice::ReadOnly) == true, "failed to open file", bRes = false, content);
        content = file.readAll();
        if(content.isEmpty() == false) TRY_BREAK;
        file.close();

        // 等待
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 200)
        {
            QCoreApplication::processEvents();
        }
    }
    return content;
}


void QmlLanguageModel::openFile(const QString & path, int revision) {
    OpenedFileManager::Instance()->openFile(path, revision);
}

void QmlLanguageModel::closeFile(const QString & path) {
    OpenedFileManager::Instance()->closeFile(path);
}

void QmlLanguageModel::updateFile(const QString & path, const QString & content, int revision) {
    OpenedFileManager::Instance()->updateFile(path, content, revision);
}


Json QmlLanguageModel::formatFile(const QString &path, uint32_t uTableSize)
{
    auto content =  OpenedFileManager::Instance()->fileContent(path);

    // 解析文档基础语法
    auto doc = Document::create(Utils::FilePath::fromString(path), Dialect::Qml);
    doc->setSource(content);
    doc->setLanguage(QmlJS::Dialect::Qml);
    doc->parse();
    if(doc->isParsedCorrectly() == false) return Json();

    // 格式化
    QString text = QmlJS::reformat(doc, uTableSize, uTableSize, 128);

    // 范围
    QTextDocument qdoc;
    qdoc.setPlainText(content);
    int line = qdoc.lineCount();
    RANGE_S range;
    range.end.line = line;

    Json out = Json::array();
    out.emplace_back(Json{
        {"range", range},
        {"newText", OwO::QStringToUtf8(text)}
    });
    return out;
}


Json QmlLanguageModel::queryColor(const QString &path)
{
    auto content =  OpenedFileManager::Instance()->fileContent(path);

    QList<RANGE_S> lstRange;
    QList<QString> lstSegment;

    QStringList lines = content.split("\n");
    for (size_t row = 0; row < lines.size(); ++row)
    {
        auto & line = lines[row];
        int start = 0;
        for(size_t col = 0 ; col < line.size() ; ++ col){
            auto & ch = line[col];

            // 左边的 "
            if(ch != '\"') continue;
            start = col;

            // 右边的 "
            while (++ col < line.size())
            {
                if(line[col] == '\"') break;
            }
            if(col >= line.size()) break;

            // #123456 或者 #12345678 
            auto segment = line.mid(start + 1, col - start - 1); 
            if((segment.size() == 7 || segment.size() == 9) && segment.startsWith("#")){
                RANGE_S range;
                range.start.line = row;
                range.start.character = start + 1;
                range.end.line = row;
                range.end.character = col;

                lstRange.push_back(range);
                lstSegment.push_back(segment.mid(1));
            }
        }
    }
    
    // 重新对数据进行筛选
    Json::array_t colors;
    for(size_t i =0 ; i < lstRange.size() ; ++i){
        auto & range = lstRange[i];
        auto & segment = lstSegment[i];

        // ARGB
        double a = 1.0,r = 1.0,g = 1.0,b = 1.0;
        bool bOk = true;
        if(segment.size() == 8){
            a = Private::ParseHexChannel("0x" + segment.mid(0,2), bOk);
            if(bOk == false) continue;

            r = Private::ParseHexChannel("0x" + segment.mid(2,2), bOk);
            if(bOk == false) continue;

            g = Private::ParseHexChannel("0x" + segment.mid(4,2), bOk);
            if(bOk == false) continue;

            b = Private::ParseHexChannel("0x" + segment.mid(6,2), bOk);
            if(bOk == false) continue;
        }else if(segment.size() == 6){
            r = Private::ParseHexChannel("0x" + segment.mid(0,2), bOk);
            if(bOk == false) continue;

            g = Private::ParseHexChannel("0x" + segment.mid(2,2), bOk);
            if(bOk == false) continue;

            b = Private::ParseHexChannel("0x" + segment.mid(4,2), bOk);
            if(bOk == false) continue;
        }

        Json color{
            {"red", r},
            {"green", g},
            {"blue", b},
            {"alpha", a}
        };

        colors.emplace_back(Json{
            {"range", range},
            {"color", color}
        });
    }

    return colors;
}

size_t QmlLanguageModel::convertPosition(const QString & content, const POSITION_S &pos)
{
    int line = pos.line;
    int column = pos.character;
    int64_t position = 0;

    for(auto & ch : content){
        if(column <= 0 && line <= 0) break;

        if( line > 0 && ch == '\n'){
            --line;
        }else if(line == 0 && column > 0){
            --column;
        }

        ++position;
    }
    
    return position;
}
