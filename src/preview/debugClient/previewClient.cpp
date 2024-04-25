#include "debugClient/previewClient.h"

#include <qmldebug/qpacketprotocol.h>

PreviewClient::PreviewClient(QmlDebug::QmlDebugConnection *connection)
    : QmlDebug::QmlDebugClient(QLatin1String("QmlPreview"), connection)
{
}

void PreviewClient::loadUrl(const QUrl &url)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Load) << url;
    sendMessage(packet.data());
}

void PreviewClient::rerun()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Rerun);
    sendMessage(packet.data());
}

void PreviewClient::zoom(float zoomFactor)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Zoom) << zoomFactor;
    sendMessage(packet.data());
}

void PreviewClient::announceFile(const QString &path, const QByteArray &contents)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(File) << path << contents;
    sendMessage(packet.data());
}

void PreviewClient::announceDirectory(const QString &path, const QStringList &entries)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Directory) << path << entries;
    sendMessage(packet.data());
}

void PreviewClient::announceError(const QString &path)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Error) << path;
    sendMessage(packet.data());
}

void PreviewClient::clearCache()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(ClearCache);
    sendMessage(packet.data());
}

void PreviewClient::messageReceived(const QByteArray &data)
{
    QmlDebug::QPacket packet(dataStreamVersion(), data);
    qint8 command;
    packet >> command;
    switch (command)
    {
    case Request:
    {
        QString path;
        packet >> path;
        emit sigPathRequested(path);
        break;
    }
    case Error:
    {
        QString error;
        packet >> error;
        emit sigErrorMessage(error);
        break;
    }
    case Fps:
    {
        FPS_INFO_S info;
        packet >> info.numSyncs >> info.minSync >> info.maxSync >> info.totalSync >> info.numRenders >> info.minRender >> info.maxRender >> info.totalRender;
        emit sigFpsInfo(info);
        break;
    }
    default:
        qDebug() << "invalid command" << command;
        break;
    }
}

void PreviewClient::stateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state == Unavailable)
    {
        emit sigDebugServiceUnavailable(name());
    }
}
