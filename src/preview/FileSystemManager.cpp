
#include "FileSystemManager.h"

#include <QMap>
#include <QFile>
#include <QString>
#include <QFileInfo>

#include "utils/qrcparser.h"

#include "common/utils.h"
#include "common/previewLog.hpp"
#include "common/previewProject.h"

using namespace Utils;
using namespace QmlJS;
using namespace ProjectExplorer;

namespace OxO
{
    static QMap<QString, QString> TraverseQrc(Utils::QrcParser::Ptr parse){
        QMap<QString, QString> res;
        auto lang = Project::Instance()->getLanguage();

        QStringList folder;
        folder << "/";

        int cursor = 0;
        while (cursor < folder.size()) {
            QString curUrl = folder.at(cursor ++);

            // 获取 curr 路径下的 qrc 目录
            QMap<QString,QStringList> content;
            parse->collectFilesInPath(curUrl, &content, true, &lang);

            for (auto & url : content.keys())
            {
                if (url.endsWith(QLatin1Char('/'))){
                    folder.append(url);
                    continue;
                }

                for (auto & path : content[url])
                {
                    res.insert(path, url);
                }
            }
        }

        return res;
    }
} // namespace Internal

FileSystemManger::FileSystemManger(QObject *parent)
    :QObject(parent)
{
    m_timer = new QTimer(this);
}

bool FileSystemManger::init()
{
    /* 创建查找器 */
    auto project = Project::Instance();
    m_pFinder = std::make_shared<Utils::FileInProjectFinder>();
    m_pFinder->setProjectDirectory(FilePath::fromString(project->getProjectFolder()));
    m_pFinder->setSysroot(FilePath::fromString(project->getLimitedFolder()));

    FilePaths paths;
    for (auto & path : project->getExtendSearchFolder())
    {
        paths.append(FilePath::fromString(path));
    }
    m_pFinder->setAdditionalSearchDirectories(paths);
    
    



    /* 连接 */
    bool bFlag = false;
    bFlag = connect(&m_watcher, &FileSystemWatcher::fileChanged, this, [this](const QString &path){
            onFileChanged(path , true);
        });
    ASSERT_RETURN(bFlag == true, "failed to connect FileSystemWatcher::fileChanged", false);


    /* 解析器 */
    for (auto & qrcFile : project->getQrcFiles())
    {
        auto parse = m_qrcs.addPath(qrcFile, QString());
        if(parse->isValid() == false){
            printErrorMessage(qrcFile, parse->errorMessages());
        }

        m_watcher.addFile(qrcFile, FileSystemWatcher::WatchModifiedDate);
    }

    updateFocusQml();
    return true;
}

void FileSystemManger::updateFocusQml()
{
    auto project = Project::Instance();

    /* 更新主 qml 路径 */    
    if(project->getFocusLocalQml().isEmpty() == false){
        auto qrcUrl = findQrcUrlByParser(project->getFocusLocalQml());
        project->setFocusQrcQml(qrcUrl);
        LOG_DEBUG("focus qml file %s", OwO::QStringToUtf8(qrcUrl).c_str());
    }
}

void FileSystemManger::addFile(const QString &strPath)
{
    if(m_watcher.watchesFile(strPath) == false){
        m_watcher.addFile(strPath, Utils::FileSystemWatcher::WatchModifiedDate);
    }
}

void FileSystemManger::onPathRequested(const QString & strPath) {
    LOG_DEBUG("request %s", OwO::QStringToUtf8(strPath).c_str());

    auto project = Project::Instance();

    auto fileHandler = [&](const Utils::FilePath &filePath, int confidence){
        if(confidence != strPath.length()){
            emit sigAnnounceError(strPath);
            LOG_DEBUG("failed to import %s", OwO::QStringToUtf8(strPath).c_str());
            return;
        }

        // 加载文件
        bool bFlag = false;
        QByteArray contents = loadFile(filePath.toFSPathString(), bFlag);
        if(bFlag == false){
            emit sigAnnounceError(strPath);
            LOG_DEBUG("failed to load file : %s", strPath.toStdString().c_str());
            return;
        }

        // 添加监控
        if(m_watcher.watchesFile(filePath) == false){
            m_watcher.addFile(filePath, FileSystemWatcher::WatchModifiedDate);
        } 

        emit sigAnnounceFile(strPath, contents);
        LOG_DEBUG("import %s", OwO::QStringToUtf8(strPath).c_str());
    };

    auto directoryHandler = [&](const QStringList &entries, int confidence){
        if(confidence != strPath.length()){
            emit sigAnnounceError(strPath);
            LOG_DEBUG("failed to import %s", OwO::QStringToUtf8(strPath).c_str());
        }else{
            emit sigAnnounceDirectory(strPath, entries);
            LOG_DEBUG("import %s", OwO::QStringToUtf8(strPath).c_str());
        }
    };

    // 通过 qrc 解析器查找 qrc路径对应的文件，没有文件夹
    bool bFlag = findSourceByParser(strPath, fileHandler, directoryHandler);
    if(bFlag == true) return;

    // 解析器不能查找的内容，由 m_pFinder 来查找
    bFlag = m_pFinder->findFileOrDirectory(FilePath::fromString(strPath), fileHandler, directoryHandler);
    if(bFlag == true) return;

    emit sigAnnounceError(strPath);
    
}



QByteArray FileSystemManger::loadFile(const QString &strPath, bool &bRes)
{
    bRes = true;

    QFile file(strPath);
    {
        RAII_DEFER(file.close());

        ASSERT_RETURN(file.exists() == true, "file is not exist", bRes = false,QByteArray());
        ASSERT_RETURN(file.open(QIODevice::ReadOnly) == true, "failed to open file" , bRes = false, QByteArray());
        return file.readAll(); 
    }
}



void FileSystemManger::onFileChanged(const QString &strPath, bool bCheck)
{
    // Project::Instance()->getUpdateInterval() 大于时间间隔才刷新
    if(bCheck == true){
        if(m_mapFileModifyTime.contains(strPath) == true){
            auto & last = m_mapFileModifyTime[strPath];
            auto  now = QFileInfo(strPath).fileTime(QFile::FileModificationTime);

            if(last.msecsTo(now) > Project::Instance()->getUpdateInterval()){
                m_mapFileModifyTime[strPath] = now;
            }else{
                return;
            }
        }else{
            m_mapFileModifyTime[strPath] = QFileInfo(strPath).fileTime(QFile::FileModificationTime);
        }
    }
    LOG_DEBUG("file change : %s", OwO::QStringToUtf8(strPath).c_str());

    bool bFlag = false;
    QByteArray contents = loadFile(strPath, bFlag);
    if(bFlag == false){
        LOG_ERROR("failed to load file : %s", strPath.toStdString().c_str());
        m_watcher.removeFile(strPath);
        m_mapFileModifyTime.remove(strPath);
        return;
    }

    // 文件类型探测 
    FILE_TYPE_E enType = inspectFileType(strPath);
    if(enType == FILE_TYPE_NONE) return;

    // 检测文件有效性
    if(checkFileValid(strPath, contents, enType) == false) return;

    // 查找 strpath 对应的 qrc 路径
    QString strQrcUrl = findQrcUrlByParser(strPath);
    if(strQrcUrl.isEmpty() == true){
        emit sigClearCache();
    }else{
        emit sigAnnounceFile(strQrcUrl, contents);
    }

    emit sigLoadUrl(Project::Instance()->getFocusQrcQml());
}

// TODO -  非 Desktop Device ，不能直接用本地路
QString FileSystemManger::findQrcUrlByParser(const QString &strSource)
{
    auto & qrcFiles = Project::Instance()->getQrcFiles();
    auto & lang = Project::Instance()->getLanguage();
    QStringList res;

    // 在 qrc 解析器中查找
    for (auto & qrc : qrcFiles)
    {
        QrcParser::ConstPtr parser = m_qrcs.parsedPath(qrc);
        parser->collectResourceFilesForSourceFile(strSource, &res , &lang);

        if(res.size() > 0){
            QString strUrl = res.first();
            if(strUrl.startsWith(QLatin1Char('/'))){
                strUrl = QLatin1Char(':') + strUrl;
            }
            return strUrl;
        } 
    }
    return strSource;
}

bool FileSystemManger::findSourceByParser(const QString &strQrc, FileHandler fileHandler, DirectoryHandler directoryHandler)
{
    QString strQrcUrl;
    if(strQrc.startsWith(QLatin1Char(':')) == true){
        strQrcUrl = strQrc.mid(1);
    }else{
        return false;
    }

    auto & qrcFiles = Project::Instance()->getQrcFiles();
    auto & lang = Project::Instance()->getLanguage();

    // 在 qrc 解析器中查找文件
    QStringList resFile;
    for (auto & qrc : qrcFiles)
    {
        QrcParser::ConstPtr parser = m_qrcs.parsedPath(qrc);
        if(parser == nullptr) continue;

        parser->collectFilesAtPath(strQrcUrl, &resFile , &lang);
        if(resFile.size() <= 0) continue;

        fileHandler(Utils::FilePath::fromFileInfo(QFileInfo(resFile[0])), strQrc.length());
        return true;
    }

    // 在 qrc 解析器中查找文件夹
    if(strQrcUrl.endsWith('/') == false){
        strQrcUrl += "/";
    }

    QMap<QString, QStringList> res;
    for (auto & qrc : qrcFiles)
    {
        QrcParser::ConstPtr parser = m_qrcs.parsedPath(qrc);
        if(parser == nullptr) continue;

        parser->collectFilesInPath(strQrcUrl, &res, true , &lang);
        if(res.size() <= 0) continue;

        directoryHandler(res[0], strQrc.length());
        return true;
    }


    return false;
}


bool FileSystemManger::checkFileValid(const QString &strSource, const QByteArray & content, const FILE_TYPE_E & enType)
{
    // 空文件不检测
    if(content.isEmpty() == true) return true;


    switch (enType)
    {
    case FILE_TYPE_E::FILE_QML_JS :{

        auto path = FilePath::fromString(strSource);
        auto dialect = ModelManagerInterface::guessLanguageOfFile(path);
        auto doc = QmlJS::Document::create(Utils::FilePath::fromString(strSource), dialect);
        ASSERT_RETURN(doc != nullptr, "创建 qmljs doc 失败", false);

        doc->setSource(QString::fromUtf8(content)); 
        if(doc->parse() == false){
            printErrorMessage(strSource, doc->diagnosticMessages());
            return false; 
        }
        return true;
    }
    case FILE_TYPE_E::FILE_QRC:{
        // 检测
        auto parse = QrcParser::parseQrcFile(strSource, QString::fromUtf8(content)); 
        if(parse->isValid() == false){
            printErrorMessage(strSource, parse->errorMessages());
            return false;
        }

        // 更新 qrc 管理器
        m_qrcs.updatePath(strSource, content);
        return true;
    }
    default:
        break;
    }
    return true;
}

void FileSystemManger::printErrorMessage(const QString & file, const QStringList &lstMsg)
{
    QString fileName = QFileInfo(file).fileName();
    for (auto msg : lstMsg)
    {
        QString str = QString("%1: %2").arg(fileName, msg);
        CONSOLE_ERROR("%s", OwO::QStringToUtf8(str).c_str());
    }
}

void FileSystemManger::printErrorMessage(const QString & file,const QList<QmlJS::DiagnosticMessage> &lstMsg)
{
    QString fileName = QFileInfo(file).fileName();
    for (auto & msg : lstMsg)
    {
        QString str = QString("[%1:%2] %3").arg(fileName, QString::number(msg.loc.startLine), msg.message);
        CONSOLE_ERROR("%s", OwO::QStringToUtf8(str).c_str());
    }
}



FileSystemManger::FILE_TYPE_E FileSystemManger::inspectFileType(const QString &strSource)
{
    // 不支持实时更新 qtquickcontrols2.conf 
    if(strSource.endsWith("qtquickcontrols2.conf") == true) return FILE_TYPE_E::FILE_TYPE_NONE;

    // qrc 文件
    if(strSource.endsWith(".qrc") == true) return FILE_TYPE_E::FILE_QRC;

    // qml 或 js 类型文件
    auto dialect = ModelManagerInterface::guessLanguageOfFile(FilePath::fromString(strSource));
    if(dialect.isQmlLikeOrJsLanguage() == true){
        return FILE_TYPE_E::FILE_QML_JS;
    }

    return FILE_TYPE_E::FILE_NORM;
}
