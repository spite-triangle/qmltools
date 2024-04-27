#ifndef PREVIEWCONNECTMANAGER_H
#define PREVIEWCONNECTMANAGER_H


#include <QObject>
#include <QPointer>

#include <qmldebug/qmldebugconnectionmanager.h>


#include "common/previewDefines.h"
#include "debugClient/previewClient.h"
#include "debugClient/TranslationClient.h"

class PreviewConnectManager : public QmlDebug::QmlDebugConnectionManager{
    Q_OBJECT

public:
    using Ptr = QPointer<PreviewConnectManager>;


public:
    explicit PreviewConnectManager(QObject *parent = nullptr);

public slots:
    void onLoadUrl(const QUrl &url);
    void onRerun();
    void onZoom(float zoomFactor);
    void onAnnounceFile(const QString &path, const QByteArray &contents);
    void onAnnounceDirectory(const QString &path, const QStringList &entries);
    void onAnnounceError(const QString &path);
    void onClearCache();
    void onDebugServiceUnavailable(const QString & name);
    void onErrorMessage(const QString & name);

signals:
    void sigPathRequested(const QString & path);
    void sigDebugServiceUnavailable(const QString & name);

protected:
    virtual void createClients() override;
    virtual void destroyClients() override;

private:
    PreviewClient::Ptr m_pPreviewClient;
    TranslationClient::Ptr m_pTranslationClient;
};



#endif /* PREVIEWCONNECTMANAGER_H */
