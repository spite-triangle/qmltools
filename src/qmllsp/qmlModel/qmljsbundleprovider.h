#ifndef QMLJSBUNDLEPROVIDER_H
#define QMLJSBUNDLEPROVIDER_H

#include <QHash>
#include <QObject>
#include <QtGlobal>
#include <QVersionNumber>

namespace ProjectExplorer {
class Project;
}

namespace QmlJS {
class QmlLanguageBundles;
class QmlBundle;
} // namespace QmlJS


class IBundleProvider : public QObject
{
    Q_OBJECT
public:
    explicit IBundleProvider(QObject *parent = nullptr);
    ~IBundleProvider() override;

    static const QList<IBundleProvider *> allBundleProviders();

    virtual void mergeBundlesForKit(QmlJS::QmlLanguageBundles &bundles
                                    , const QHash<QString,QString> &replacements) = 0;
};

class  BasicBundleProvider : public IBundleProvider
{
    Q_OBJECT
public:
    explicit BasicBundleProvider(QObject *parent = nullptr);

    void mergeBundlesForKit(QmlJS::QmlLanguageBundles &bundles,
                            const QHash<QString,QString> &replacements) override;

    static QmlJS::QmlBundle defaultBundle(const QString &bundleInfoName, const QVersionNumber & version, bool stripVersions = true);
    static QmlJS::QmlBundle defaultQt5QtQuick2Bundle(const QVersionNumber & version);
    static QmlJS::QmlBundle defaultQbsBundle();
    static QmlJS::QmlBundle defaultQmltypesBundle();
    static QmlJS::QmlBundle defaultQmlprojectBundle();
};

#endif // QMLJSBUNDLEPROVIDER_H
