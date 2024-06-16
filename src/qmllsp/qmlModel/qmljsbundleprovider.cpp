// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsbundleprovider.h"

#include <QDir>

#include <qmljs/qmljsbundle.h>
#include <qmljs/qmljsconstants.h>

#include "common/lspProject.h"


using namespace QmlJS;

/*!
  \class QmlJSEditor::BasicBundleProvider

    \brief The BasicBundleProvider class sets up the default bundles for Qt and
    various QML states.
  */
BasicBundleProvider::BasicBundleProvider(QObject *parent) :
    IBundleProvider(parent)
{ }

QmlBundle BasicBundleProvider::defaultBundle(const QString &bundleInfoName,const QVersionNumber & version, bool stripVersions)
{
    static bool wroteErrors = false;
    QmlBundle res;

    auto project = ProjectExplorer::Project::Instance();
    const Utils::FilePath defaultBundlePath = Utils::FilePath::fromString(project->getQmlTypeDescription()) / bundleInfoName;
    if (!defaultBundlePath.exists()) {
        qWarning() << "BasicBundleProvider: ERROR " << defaultBundlePath
                   << " not found";
        return res;
    }
    QStringList errors;
    stripVersions = stripVersions && version.majorVersion() > 5;
    if (!res.readFrom(defaultBundlePath.toString(), stripVersions, &errors) && !wroteErrors) {
        qWarning() << "BasicBundleProvider: ERROR reading " << defaultBundlePath
                   << " : " << errors;
        wroteErrors = true;
    }
    return res;
}

QmlBundle BasicBundleProvider::defaultQt5QtQuick2Bundle(const QVersionNumber & version)
{
    QmlBundle result = defaultBundle(QLatin1String("qt5QtQuick2-bundle.json"), version);
    if (version.majorVersion() < 6)
        return result;

    //  macos 
    // result.merge(defaultBundle(QLatin1String("qt5QtQuick2ext-macos-bundle.json"), version));
    result.merge(defaultBundle(QLatin1String("qt5QtQuick2ext-win-bundle.json"), version));
    return result;
}

QmlBundle BasicBundleProvider::defaultQbsBundle()
{
    return defaultBundle(QLatin1String("qbs-bundle.json"), QVersionNumber(), false);
}

QmlBundle BasicBundleProvider::defaultQmltypesBundle()
{
    return defaultBundle(QLatin1String("qmltypes-bundle.json"), QVersionNumber(), false);
}

QmlBundle BasicBundleProvider::defaultQmlprojectBundle()
{
    return defaultBundle(QLatin1String("qmlproject-bundle.json"), QVersionNumber(), false);
}

void BasicBundleProvider::mergeBundlesForKit(QmlLanguageBundles &bundles
                                             , const QHash<QString,QString> &replacements)
{
    QHash<QString,QString> myReplacements = replacements;

    bundles.mergeBundleForLanguage(Dialect::QmlQbs, defaultQbsBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlTypeInfo, defaultQmltypesBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlProject, defaultQmlprojectBundle());

    auto project = ProjectExplorer::Project::Instance();
    auto qtVersion = project->getVersionSdk();
    if (qtVersion.isNull() == true) {
        QmlBundle b2(defaultQt5QtQuick2Bundle(qtVersion));
        bundles.mergeBundleForLanguage(Dialect::Qml, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, b2);
        return;
    }
    QString qtQmlPath = project->sdkAssetPath(ProjectExplorer::Project::SDK_ASSET_E::SDK_FOLDER_QML);

    myReplacements.insert(QLatin1String("$(CURRENT_DIRECTORY)"), qtQmlPath);
    QDir qtQuick2Bundles(qtQmlPath);
    qtQuick2Bundles.setNameFilters(QStringList(QLatin1String("*-bundle.json")));
    QmlBundle qtQuick2Bundle;
    QFileInfoList list = qtQuick2Bundles.entryInfoList();
    bool stripVersions = qtVersion.majorVersion() > 5;
    for (int i = 0; i < list.size(); ++i) {
        QmlBundle bAtt;
        QStringList errors;
        if (!bAtt.readFrom(list.value(i).filePath(), stripVersions, &errors))
            qWarning() << "BasicBundleProvider: ERROR reading " << list[i].filePath() << " : "
                       << errors;
        qtQuick2Bundle.merge(bAtt);
    }
    if (!qtQuick2Bundle.supportedImports().contains(QLatin1String("QtQuick 2."),
                                                    PersistentTrie::Partial)) {
        qtQuick2Bundle.merge(defaultQt5QtQuick2Bundle(qtVersion));
    }
    qtQuick2Bundle.replaceVars(myReplacements);
    bundles.mergeBundleForLanguage(Dialect::Qml, qtQuick2Bundle);
    bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, qtQuick2Bundle);
    bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, qtQuick2Bundle);

}

static QList<IBundleProvider *> g_bundleProviders;

IBundleProvider::IBundleProvider(QObject *parent)
    : QObject(parent)
{
    g_bundleProviders.append(this);
}

IBundleProvider::~IBundleProvider()
{
    g_bundleProviders.removeOne(this);
}

const QList<IBundleProvider *> IBundleProvider::allBundleProviders()
{
    return g_bundleProviders;
}

