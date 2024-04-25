
#include "previewTool.h"

#include <QUrl>
#include <QCoreApplication>

#include "common/previewProject.h"

PreviewTool::PreviewTool(QObject * parent) {
    m_bQuit = false;
}

bool PreviewTool::init() {
    /* 创建 */
    m_fileSystemManager = new FileSystemManger(this);
    ASSERT_RETURN(m_fileSystemManager != nullptr, "m_fileSystemManager == nullptr", false);
    ASSERT_RETURN(m_fileSystemManager->init() == true, "failed to init fileSystemManager", false);

    m_connectManager  = new PreviewConnectManager(this);
    ASSERT_RETURN(m_connectManager != nullptr, "m_connectManager == nullptr", false);

    /* 连接 */
    bool bFlag = false;

    // 文件系统
    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigLoadUrl, 
                    m_connectManager, &PreviewConnectManager::onLoadUrl);
    ASSERT_RETURN(bFlag == true, "failed to connect sigLoadUrl", false);

    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigClearCache, 
                    m_connectManager, &PreviewConnectManager::onClearCache);
    ASSERT_RETURN(bFlag == true, "failed to connect sigClearCache", false);

    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigAnnounceFile, 
                    m_connectManager, &PreviewConnectManager::onAnnounceFile);
    ASSERT_RETURN(bFlag == true, "failed to connect sigAnnounceFile", false);

    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigAnnounceError, 
                    m_connectManager, &PreviewConnectManager::onAnnounceError);
    ASSERT_RETURN(bFlag == true, "failed to connect sigAnnounceError", false);

    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigAnnounceDirectory, 
                    m_connectManager, &PreviewConnectManager::onAnnounceDirectory);
    ASSERT_RETURN(bFlag == true, "failed to connect sigAnnounceDirectory", false);

    bFlag = connect(m_fileSystemManager, &FileSystemManger::sigRerun, 
                    m_connectManager, &PreviewConnectManager::onRerun);
    ASSERT_RETURN(bFlag == true, "failed to connect sigRerun", false);

    // debug 客户端连接目标程序
    bFlag = connect(m_connectManager, &PreviewConnectManager::sigPathRequested, 
                    m_fileSystemManager, &FileSystemManger::onPathRequested);
    ASSERT_RETURN(bFlag == true, "failed to connect sigPathRequested", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::sigDebugServiceUnavailable, 
                    this, &PreviewTool::onDebugServiceUnavailable);
    ASSERT_RETURN(bFlag == true, "failed to connect sigDebugServiceUnavailable", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::connectionOpened, this, [this](){
        LOG_DEBUG("success to connect");

        // 激活需要预览的 qml 文件
        auto project = ProjectExplorer::Project::Instance();
        m_connectManager->onLoadUrl(project->getFocusQrcQml());
    });
    ASSERT_RETURN(bFlag == true, "failed to connect connectionOpened", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::connectionFailed, [this](){
        LOG_DEBUG("failed to connect");
        QCoreApplication::quit();
    });
    ASSERT_RETURN(bFlag == true, "failed to connect connectionFailed", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::connectionClosed, [this](){
        LOG_DEBUG("connect closed");
        QCoreApplication::quit();
    });
    ASSERT_RETURN(bFlag == true, "failed to connect connectionClosed", false);

    return true;

}


void PreviewTool::connectServer(const QString &strServer)
{
    ASSERT_RETURN(m_connectManager != nullptr, "m_connectManager == nullptr");
    
    m_connectManager->connectToServer(QUrl(strServer));
}

void PreviewTool::onApplicationQuit()
{
    disconnect();

    ASSERT_RETURN(m_connectManager != nullptr, "m_connectManager == nullptr");
    ASSERT_RETURN(m_fileSystemManager != nullptr, "m_connectManager == nullptr");

    if(m_connectManager->isConnected() == true){
        m_connectManager->disconnectFromServer();
    }

    m_connectManager->disconnect();
    m_fileSystemManager->disconnect();
}


void PreviewTool::onDebugServiceUnavailable(const QString & name) {
    QCoreApplication::quit();
}