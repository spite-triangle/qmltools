#include "configTask.h"

#include <QFileInfo>

#include "common/utils.h"
#include "common/lspProject.h"
#include "server/lspServer.h"
#include "qmlModel/qmlLanguageModel.h"

namespace Private
{
    struct QML_SETTINGS_S{
        std::string sdk;
        std::vector<std::string> qrc;
        std::vector<std::string> targetFolder;
        std::vector<std::string> importPath;
        std::vector<std::string> qml2ImportPath;
        std::vector<std::string> sourceFolder;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(QML_SETTINGS_S, 
                                            sdk, qrc, targetFolder, importPath, qml2ImportPath, sourceFolder);
} // namespace Private


/* 
    {
        "jsonrpc": "2.0",
        "method": "workspace/didChangeConfiguration",
        "params": {
            "sdk": string,
            "qrc" : string[],
            "targetFolder" : string[],
            "importPath" : string[],
            "qml2ImportPath": string[],
            "sourceFolder" : string[]
        }
    }
 */
bool ConfigTask::handleNotification(const Json &req)
{
    Private::QML_SETTINGS_S settings = req["params"];

    auto model = QmlLanguageModel::Instance();
    auto project = ProjectExplorer::Project::Instance();

    bool bChanged = false;
    if(settings.sdk.empty() == false){
        QFileInfo folder(OwO::Utf8ToQString(settings.sdk));
        if(folder.exists() != true || folder.isDir() != true) return true;
        project->setSdkFolder(folder.absoluteFilePath());
        bChanged = true;
    }

    QStringList lstQrc;
    for(auto & item : settings.qrc){
        QFileInfo folder(OwO::Utf8ToQString(item));
        if(folder.exists() != true || folder.isFile() != true) continue;
        lstQrc.push_back(folder.absoluteFilePath());
    }
    if(lstQrc.size() > 0){
        project->setQrcFiles(lstQrc);
        bChanged = true;
    }

    QStringList lstImport;
    for(auto & item : settings.importPath){
        QFileInfo folder(OwO::Utf8ToQString(item));
        if(folder.exists() != true || folder.isDir() != true) continue;
        lstImport.push_back(folder.absoluteFilePath());
    }
    if(lstImport.size() > 0){
        project->setImportPaths(lstImport);
        bChanged = true;
    }

    QStringList lstQml2Import;
    for(auto & item : settings.qml2ImportPath){
        QFileInfo folder(OwO::Utf8ToQString(item));
        if(folder.exists() != true || folder.isDir() != true) continue;
        lstQml2Import.push_back(folder.absoluteFilePath());
    }
    if(lstQml2Import.size() > 0){
        project->setQml2ImportPath(lstQml2Import);
        bChanged = true;
    }

    QStringList lstSource;
    for(auto & item : settings.sourceFolder){
        QFileInfo folder(OwO::Utf8ToQString(item));
        if(folder.exists() != true || folder.isDir() != true) continue;
        lstSource.push_back(folder.absoluteFilePath());
    }
    if(lstSource.size() > 0){
        project->setSourceFolder(lstSource);
        bChanged = true;
    }

    QStringList lstTarget;
    for(auto & item : settings.targetFolder){
        QFileInfo folder(OwO::Utf8ToQString(item));
        if(folder.exists() != true || folder.isDir() != true) continue;
        lstTarget.push_back(folder.absoluteFilePath());
    }
    if(lstTarget.size() > 0){
        project->setTargetFolder(lstTarget);
        bChanged = true;
    }

    if(bChanged == true){
        model->restProjectInfo();
        model->waitModleUpdate();
        model->resetModle();
    }
    return true;
}
