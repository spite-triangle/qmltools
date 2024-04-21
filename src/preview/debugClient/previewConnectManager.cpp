

#include "debugClient/previewConnectManager.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "common/utils.h"
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
    static  QRegularExpression reg(R"(qrc(:\S+):)");

    QString url;
    QStringList msgs = strMsgText.split('\n');
    for (auto & msg : msgs)
    {
        QString line = msg.simplified(); 
        if (line.endsWith("No such file or directory") == true)
        {
            QRegularExpressionMatch match = reg.match(line);
            if(match.hasMatch() != true) continue;

            url = match.captured(1);
            break;
        }
    }

    if(url.isEmpty() == false){
        emit sigPathRequested(url, true);
    }

    CONSOLE_ERROR("%s", OwO::QStringToUtf8(strMsgText.simplified()).c_str());
}
