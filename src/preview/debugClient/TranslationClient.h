#ifndef TRANSLATIONCLIENT_H
#define TRANSLATIONCLIENT_H

#include <qmldebug/qmldebugclient.h>


class  TranslationClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    using Ptr = QPointer<TranslationClient>;

    explicit TranslationClient(QmlDebug::QmlDebugConnection *connection);

    void changeLanguage(const QUrl &url, const QString &localeIsoCode);
    void stateChanged(State state) override;

signals:
    void sigDebugServiceUnavailable(const QString & name);
};

#endif /* TRANSLATIONCLIENT_H */
