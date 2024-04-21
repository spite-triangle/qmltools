#ifndef PREVIEWCLIENT_H
#define PREVIEWCLIENT_H

#include <QUrl>
#include <QString>
#include <QObject>


#include <qmldebug/qmldebugclient.h>

#include "common/defines.h"
#include "common/previewDefines.h"

class PreviewClient : public QmlDebug::QmlDebugClient{

    Q_OBJECT
public:
    using Ptr = QPointer<PreviewClient>;

    enum COMMAND_E {
        File,
        Load,
        Request,
        Error,
        Rerun,
        Directory,
        ClearCache,
        Zoom,
        Fps
    };

    struct FPS_INFO_S{
        quint16 numSyncs = 0;
        quint16 minSync = std::numeric_limits<quint16>::max();
        quint16 maxSync = 0;
        quint16 totalSync = 0;

        quint16 numRenders = 0;
        quint16 minRender = std::numeric_limits<quint16>::max();
        quint16 maxRender = 0;
        quint16 totalRender = 0;
    };

    explicit PreviewClient(QmlDebug::QmlDebugConnection *connection);

    void loadUrl(const QUrl &url);
    void rerun();
    void zoom(float zoomFactor);
    void announceFile(const QString &path, const QByteArray &contents);
    void announceDirectory(const QString &path, const QStringList &entries);
    void announceError(const QString &path);
    void clearCache();

    void messageReceived(const QByteArray &message) override;
    void stateChanged(State state) override;

signals:
    void sigDebugServiceUnavailable(const QString & name);
    void sigPathRequested(const QString & path, const bool & bReload);
    void sigErrorMessage(const QString & msg);
    void sigFpsInfo(const FPS_INFO_S & stInfo);
};



#endif /* PREVIEWCLIENT_H */
