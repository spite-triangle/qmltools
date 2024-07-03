
#include "qmljsmodelmanager.h"

#include "utils/mimeutils.h"
#include "utils/mimeconstants.h"

#include "qmlModel/qmljsOpenedFileManager.h"

namespace QmlJS {

ModelManager::ModelManager(QObject *parent)
    : ModelManagerInterface(parent)
{
}

QHash<QString, Dialect> ModelManager::initLanguageForSuffix() const
{
    using namespace Utils;

    QHash<QString,Dialect> res = ModelManagerInterface::languageForSuffix();

    MimeType jsSourceTy = Utils::mimeTypeForName(Utils::Constants::JS_MIMETYPE);
    const QStringList jsSuffixes = jsSourceTy.suffixes();
    for (const QString &suffix : jsSuffixes)
        res[suffix] = Dialect::JavaScript;
    MimeType qmlSourceTy = Utils::mimeTypeForName(Utils::Constants::QML_MIMETYPE);
    const QStringList qmlSuffixes = qmlSourceTy.suffixes();
    for (const QString &suffix : qmlSuffixes)
        res[suffix] = Dialect::Qml;
    MimeType qbsSourceTy = Utils::mimeTypeForName(Utils::Constants::QBS_MIMETYPE);
    const QStringList qbsSuffixes = qbsSourceTy.suffixes();
    for (const QString &suffix : qbsSuffixes)
        res[suffix] = Dialect::QmlQbs;
    MimeType qmlProjectSourceTy = Utils::mimeTypeForName(Utils::Constants::QMLPROJECT_MIMETYPE);
    const QStringList qmlProjSuffixes = qmlProjectSourceTy.suffixes();
    for (const QString &suffix : qmlProjSuffixes)
        res[suffix] = Dialect::QmlProject;
    MimeType qmlUiSourceTy = Utils::mimeTypeForName(Utils::Constants::QMLUI_MIMETYPE);
    const QStringList qmlUiSuffixes = qmlUiSourceTy.suffixes();
    for (const QString &suffix : qmlUiSuffixes)
        res[suffix] = Dialect::QmlQtQuick2Ui;
    MimeType jsonSourceTy = Utils::mimeTypeForName(Utils::Constants::JSON_MIMETYPE);
    const QStringList jsonSuffixes = jsonSourceTy.suffixes();
    for (const QString &suffix : jsonSuffixes)
        res[suffix] = Dialect::Json;
    return res;
}



QHash<QString, QmlJS::Dialect> ModelManager::languageForSuffix() const
{
    static QHash<QString, QmlJS::Dialect> res = initLanguageForSuffix();
    return res;
}

ModelManager::WorkingCopy ModelManager::workingCopyInternal() const {
    WorkingCopy res;

    OpenedFileManager::Table_t table = OpenedFileManager::Instance()->getOpenedFiles();
    for(auto & key : table.keys()){
        res.insert(key, table[key].first, table[key].second);
    }

    return res;
}

}