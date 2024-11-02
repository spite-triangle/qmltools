
#include "previewTool.h"

#include <QUrl>
#include <QCoreApplication>

#include <iostream>

#include "common/utils.h"
#include "common/previewLog.h"
#include "common/previewProject.h"

bool PreviewTool::m_bQuit = false;

PreviewTool::PreviewTool(QObject * parent) {
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
    
    // 交互子线程
    bFlag = connect(this, &PreviewTool::sigFocusChange, m_fileSystemManager, 
        [&](const QString & path){
            ASSERT_RETURN(m_fileSystemManager != nullptr, "m_fileSystemManager == nullptr");
            m_fileSystemManager->onFileChanged(path);
        });
    ASSERT_RETURN(bFlag == true, "failed to connect sigFocusChange", false);

    bFlag = connect(this, &PreviewTool::sigRerun,
                   m_connectManager, &PreviewConnectManager::onRerun);
    ASSERT_RETURN(bFlag == true, "failed to connect sigRerun", false);

    bFlag = connect(this, &PreviewTool::sigZoom,
                   m_connectManager, &PreviewConnectManager::onZoom);
    ASSERT_RETURN(bFlag == true, "failed to connect sigZoom", false);


    // debug 客户端连接目标程序
    bFlag = connect(m_connectManager, &PreviewConnectManager::sigPathRequested, 
                    m_fileSystemManager, &FileSystemManger::onPathRequested);
    ASSERT_RETURN(bFlag == true, "failed to connect sigPathRequested", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::sigErrorPathRequested, 
                    m_fileSystemManager, &FileSystemManger::onErrorPathRequested);
    ASSERT_RETURN(bFlag == true, "failed to connect sigErrorPathRequested", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::sigDebugServiceUnavailable, 
                    this, &PreviewTool::onDebugServiceUnavailable);
    ASSERT_RETURN(bFlag == true, "failed to connect sigDebugServiceUnavailable", false);

    bFlag = connect(m_connectManager, &PreviewConnectManager::connectionOpened, this, [this](){
        LOG_DEBUG("success to connect");

        // 激活需要预览的 qml 文件
        auto project = ProjectExplorer::Project::Instance();
        m_connectManager->onLoadUrl(project->getFocusQrcQml());
        m_connectManager->onZoom(project->getZoom());

        /* 创建一个交互线程 */
        m_pInterface = std::shared_ptr<std::thread>(new std::thread(&PreviewTool::runInterface, this), PreviewTool::closeThread);
        ASSERT_RETURN(m_pInterface != nullptr, "failed to create a interface thread.")
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

void PreviewTool::initCommands()
{
    m_commandManager.add("help", "Commands description.", [&](){
        INTERFACE_DEBUG("%s", m_commandManager.usage().c_str());
    });

    m_commandManager.add("refresh", "Reflash preview interface.", [&](){
        emit sigRerun();
        INTERFACE_DEBUG("");
    });

    m_commandManager.add<float>("zoom", "Display zoom factor.", [&](const float & val){
        emit sigZoom(val);
        INTERFACE_DEBUG("");
    });

    m_commandManager.add("fps", "Launch show Interface FPS Infomation.", [&](){
        ProjectExplorer::Project::Instance()->setShowFPS(true);
        INTERFACE_DEBUG("");
    });

    m_commandManager.add("s", "Stop show Interface FPS Infomation.", [&](){
        ProjectExplorer::Project::Instance()->setShowFPS(false);
        INTERFACE_DEBUG("");
    });

    m_commandManager.add<float>("t", "Reflash interval of FPS information. Minium interval value is 1 sec.", [&](const float & val){
        if(val >= 0.99){
            ProjectExplorer::Project::Instance()->setFpsInterval(val);
        }
        INTERFACE_DEBUG("");
    });

    m_commandManager.add("clc", "Clear Console.", [&](){
        INTERFACE_DEBUG(FORMAT_CLEAR);
    });

    INTERFACE_DEBUG("%s", m_commandManager.usage().c_str());
}

void PreviewTool::runInterface()
{
    initCommands();

    std::string in;
    while (std::getline(std::cin, in))
    {
        m_commandManager.parse(std::move(in));
    }
}

void PreviewTool::closeThread(std::thread *p)
{
    if(p == nullptr) return;
    if(p->joinable() == true){
        p->detach();
    }
}
