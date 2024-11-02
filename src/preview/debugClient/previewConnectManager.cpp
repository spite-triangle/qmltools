

#include "debugClient/previewConnectManager.h"

#include <iomanip>

#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "common/utils.h"
#include "common/previewLog.h"
#include "common/previewProject.h"
#include "previewConnectManager.h"

PreviewConnectManager::PreviewConnectManager(QObject *parent)
    : QmlDebug::QmlDebugConnectionManager(parent)
{
    
}


void PreviewConnectManager::createClients()
{
    /* 创建客户端 */
    m_pPreviewClient = new PreviewClient(connection());
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");

    m_pTranslationClient = new TranslationClient(connection());
    ASSERT_RETURN(m_pTranslationClient != nullptr, "m_pTranslationClient == nullptr");

    /* 连接 */
    bool bFlag = false;
    bFlag = connect(m_pPreviewClient.data(), &PreviewClient::sigPathRequested, 
                    this, &PreviewConnectManager::sigPathRequested);
    ASSERT_RETURN(bFlag == true, "failed to connect sigPathRequested");

    bFlag = connect(m_pPreviewClient.data(), &PreviewClient::sigFpsInfo, 
                    this, &PreviewConnectManager::onFpsInfo);
    ASSERT_RETURN(bFlag == true, "failed to connect sigFpsInfo");

    bFlag = connect(m_pPreviewClient.data(), &PreviewClient::sigErrorMessage, 
                    this, &PreviewConnectManager::onErrorMessage);
    ASSERT_RETURN(bFlag == true, "failed to connect sigErrorMessage");

    bFlag = connect(m_pPreviewClient.data(), &PreviewClient::sigDebugServiceUnavailable, 
                    this, &PreviewConnectManager::onDebugServiceUnavailable);
    ASSERT_RETURN(bFlag == true, "failed to connect sigDebugServiceUnavailable");

    bFlag = connect(m_pTranslationClient.data(), &TranslationClient::sigDebugServiceUnavailable, 
                    this, &PreviewConnectManager::onDebugServiceUnavailable);
    ASSERT_RETURN(bFlag == true, "failed to connect sigDebugServiceUnavailable");

}

void PreviewConnectManager::destroyClients()
{
    if(m_pPreviewClient){
        m_pPreviewClient->deleteLater();
    }

    if(m_pTranslationClient){
        m_pTranslationClient->deleteLater();
    }
}
void PreviewConnectManager::onLoadUrl(const QUrl &url) {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->loadUrl(url);
}
void PreviewConnectManager::onAnnounceFile(const QString &path, const QByteArray &contents) {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->announceFile(path,contents);
}

void PreviewConnectManager::onAnnounceDirectory(const QString &path, const QStringList &entries) {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->announceDirectory(path, entries);
}

void PreviewConnectManager::onAnnounceError(const QString &path) {

    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->announceError(path);
}

void PreviewConnectManager::onClearCache() {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->clearCache();
}

void PreviewConnectManager::onRerun() {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->rerun();
}

void PreviewConnectManager::onZoom(float zoomFactor) {
    ASSERT_RETURN(m_pPreviewClient != nullptr, "m_pPreviewClient == nullptr");
    m_pPreviewClient->zoom(zoomFactor);
}

void PreviewConnectManager::onDebugServiceUnavailable(const QString &name)
{
    emit sigDebugServiceUnavailable(name);
}

void PreviewConnectManager::onErrorMessage(const QString & strMsgText)
{
    static  QRegularExpression reg(R"((\S+):-1)");

    QStringList msgs = strMsgText.split('\n');
    for (auto & msg : msgs)
    {
        QString line = msg.simplified(); 
        if(line.isEmpty()) continue;
        CONSOLE_ERROR("%s", OwO::QStringToUtf8(line).c_str());

        if(line.contains("No QML engines found") == true){
            CONSOLE_ERROR("%s", "You should recompile your project to ensure the target executable file can run correctly.");
        }

        // 重新加载空文件
        QRegularExpressionMatch match = reg.match(line);
        if(match.hasMatch() != true) continue;

        // 统一路径格式
        QString path = match.captured(1); 
        if(path.startsWith("qrc:/")){
           path = path.mid(3); // 删除 qrc 留下 :/...
        }
        emit sigErrorPathRequested(path);
    }
}

void PreviewConnectManager::onFpsInfo(const PreviewClient::FPS_INFO_S &stInfo)
{
    static QDateTime latest = QDateTime::currentDateTime();

    std::string format;
    auto project = ProjectExplorer::Project::Instance();
    auto logger = OwO::Logger::Instance();

    if(project->getShowFPS() == true){
        // 第一次需要多换一行
        if(logger->getNeedNewLine() == true){
            format = OwO::Formats( "\n" FORMAT_CURSOR_UP(1) FORMAT_SPACE "\r    ", ARG_SPACE(40));
            logger->setNeedNewLine(false);
        }else{
            format = OwO::Formats(FORMAT_CURSOR_UP(1) FORMAT_SPACE "\r    ", ARG_SPACE(40));
        }
    }else{
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    if(latest.msecsTo(now) < project->getFpsInterval() * 1000){
        return; 
    }
    latest = now;

    INTERFACE_DEBUG("%s", 
                    OwO::Format(
                        format,
                        std::setw(20), std::left,OwO::Format(std::setw(13), std::left,"totalRender: ",stInfo.totalRender),
                        std::setw(20), std::left,OwO::Format(std::setw(13), std::left,"Renders: ",stInfo.totalRender),
                        std::setw(20), std::left,OwO::Format(std::setw(13), std::left,"totalSync: ",stInfo.totalRender),
                        std::setw(20), std::left,OwO::Format(std::setw(13), std::left,"Syncs: ",stInfo.totalRender)
                    ).c_str());
}
