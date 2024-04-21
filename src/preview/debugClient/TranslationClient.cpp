#include "debugClient/TranslationClient.h"

#include <QUrl>
#include <qmldebug/qpacketprotocol.h>

// REVIEW - 不知道怎么动态获取程序的 localeIsoCode，因此直接在 Project::PROJECT_SETTING_S 中初始化设置时写死
TranslationClient::TranslationClient(QmlDebug::QmlDebugConnection *connection)
    :QmlDebug::QmlDebugClient(QLatin1String("DebugTranslation"), connection)
{
}

void TranslationClient::changeLanguage(const QUrl &url, const QString &localeIsoCode)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    const int request_change_language = 1;
    packet << request_change_language << url << localeIsoCode;
    sendMessage(packet.data());
}

void TranslationClient::stateChanged(State state)
{
    if (state == Unavailable)
        emit sigDebugServiceUnavailable(name());
}
