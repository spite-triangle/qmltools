#include "qmlLanguageModel.h"

#include <stdlib.h>

#include <QFileInfo>


#include "utils/qrcparser.h"
#include "utils/mimeutils.h"
#include "utils/algorithm.h"
#include "utils/mimeconstants.h"

#include "common/utils.h"
#include "common/lspLog.hpp"
#include "common/lspProject.h"
#include "common/lspDefine.h"

using namespace QmlJS;

namespace Private
{

bool HasRelocatableFlag(const QVersionNumber & version){
    return (version > QVersionNumber(4, 8, 4) && version < QVersionNumber(5, 0, 0)) 
            || version >= QVersionNumber(5, 1, 0);
}


} // namespace Internal

QmlLanguageModel::QmlLanguageModel(QObject *parent)
    : QObject(parent)
{
    auto modelManager = new ModelManagerInterface();

    connect(modelManager, &QmlJS::ModelManagerInterface::documentUpdated, this, &QmlLanguageModel::onDocumentUpdated);
    connect(modelManager, &QmlJS::ModelManagerInterface::projectInfoUpdated, this, &QmlLanguageModel::onProjectInfoUpdated);
}

bool QmlLanguageModel::updateProjectInfo()
{
    auto project = ProjectExplorer::Project::Instance();
    auto modelManager = ModelManagerInterface::instance();

    auto projectInfo = creatProjectInfo();

    /* 更新完毕后触发 ModelManagerInterface::projectInfoUpdated */
    modelManager->updateProjectInfo(projectInfo, project.get());
    return true;
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
    auto sourceFolders = project->getSourceFolder();
    if(sourceFolders.size() <= 0){
        projectInfo.sourceFiles = querySources(project->getProjectFolder());
    }else{
        projectInfo.sourceFiles = querySources(sourceFolders);
    }

    /* 去重 */
    projectInfo.sourceFiles = Utils::filteredUnique(projectInfo.sourceFiles);
    projectInfo.allResourceFiles = Utils::filteredUnique(projectInfo.allResourceFiles);
    projectInfo.applicationDirectories = Utils::filteredUnique(projectInfo.applicationDirectories);

    return projectInfo;
}

bool QmlLanguageModel::updateSourceFile(const QString &strFile)
{
    auto modelManager = ModelManagerInterface::instance();
    ASSERT_RETURN(modelManager != nullptr, "modelManager == nullptr" ,false);

    // qrc 文件
    if(strFile.endsWith(".qrc") == true){
        auto parse = Utils::QrcParser::parseQrcFile(strFile, QString());
        auto msg = parse->errorMessages();
        if(parse->isValid() == false){
            
            // TODO - 发送错误信息

            return false;
        }
    }

    modelManager->updateSourceFiles({Utils::FilePath::fromString(strFile)}, false);
    return true;
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

void QmlLanguageModel::onDocumentUpdated(QmlJS::Document::Ptr doc) {
    if (doc->ast()) {
        // 语法正确，需要更新 m_currentSemantic
        QmlJS::SemanticInfo info;

        setCurrentSemantic(info);
    }

    // 错误信息
    emit sigDiagnosticMessageUpdated(doc->diagnosticMessages());
}

void QmlLanguageModel::onProjectInfoUpdated(const ProjectInfo &pinfo) {

}