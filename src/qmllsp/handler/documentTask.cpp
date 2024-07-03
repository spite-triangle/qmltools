#include "documentTask.h"

#include <QUrl>

#include "common/utils.h"
#include "common/lspProject.h"
#include "server/lspServer.h"
#include "qmlModel/qmlLanguageModel.h"

/* 
    {
        "jsonrpc": "2.0",
        "method": "textDocument/didOpen",
        "params": {
            "textDocument":{
                "uri" : string,
                "languageId": string,
                "version": int,
                "text" : string  // 内容
            }
        }
    }
 */
bool DocumentOpenedTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();
    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));
    auto strPath = url.toLocalFile();

    if(url.scheme() == "file" && (strPath.endsWith(".qml", Qt::CaseInsensitive) || strPath.endsWith(".js", Qt::CaseInsensitive))){
        model->openFile(strPath);
    }
    return true;
}

/* 
    {
        "jsonrpc": "2.0",
        "method": "textDocument/didClose",
        "params": {
            "textDocument":{
                "uri" : string,
            }
        }
    }
 */
bool DocumentClosedTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();
    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));
    auto strPath = url.toLocalFile();

    if(url.scheme() == "file" && (strPath.endsWith(".qml", Qt::CaseInsensitive) || strPath.endsWith(".js", Qt::CaseInsensitive))){
        model->closeFile(strPath);
    }

    return true;
}


/* 
    {
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument":{
                "uri" : string,
                "version": int,
            },
            // 描述文本内容改变的轨迹，修改过程与数组顺序一致
            "contentChanges":[
                {
                    "range":{
                        "start":{"line": uint, "charactor": uint},
                        "end" : {"line": uint, "charactor": uint}
                    }
                    "rangeLength"?: uint,
                    "text": string  # range 范围内，修改后的内容
                }|{
                    "text": string  # 修改后的全部内容
                }
            ]
        }
    }
 */
bool DocumentChangedTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));

    if(url.scheme() == "file"){
        auto strPath = url.toLocalFile();
        auto text = req["params"]["contentChanges"][0]["text"].get<std::string>();
        model->waitModleUpdate();

        if(model->isValid()){
            model->updateFile(strPath, OwO::Utf8ToQString(text));
            model->setCurrFocusFile(strPath);
            model->updateSourceFile(strPath);
        }
    }
    return false;
}


/* 
    {
        "jsonrpc": "2.0",
        "method": "textDocument/didSave",
        "params": {
            "textDocument":{
                "uri" : string
            }
        }
    }
 */
bool DocumentSavedTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));

    if(url.scheme() == "file"){
        auto strPath = url.toLocalFile();
        model->waitModleUpdate();

        if(model->isValid()){
            model->setCurrFocusFile(strPath);
            model->updateSourceFile(strPath);
        }
    }
    return true;
}


/* 
    {
        "jsonrpc": "2.0",
        "method": "workspace/didCreateFiles",
        "params": {
            "files":[
                {uri: string}
            ]
        }
    }
 */
bool DocumentCreateTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();

    bool bRest = false;
    QStringList lstPath;
    for (auto & file : req["params"]["files"])
    {
        auto uri = file["uri"].get<std::string>();
        QUrl url = QUrl(OwO::Utf8ToQString(uri));
        if(url.scheme() == "file"){
            auto path = url.toLocalFile();
            lstPath.append(path); 
            if(path.endsWith("qmldir")) bRest = true;
        }
    }

    model->appendSourceFile(lstPath);
    
    if(bRest == true){
        model->waitModelManagerUpdate();
        model->resetModle();
    }
    return true;
}

/* 
    {
        "jsonrpc": "2.0",
        "method": "workspace/didDeleteFiles",
        "params": {
            "files":[
                {uri: string}
            ]
        }
    }
 */
bool DocumentRemoveTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();
    auto project = ProjectExplorer::Project::Instance();

    for (auto & file : req["params"]["files"])
    {
        auto uri = file["uri"].get<std::string>();
        QUrl url = QUrl(OwO::Utf8ToQString(uri));
        if(url.scheme() == "file"){
            auto path = url.toLocalFile();
            project->removeSourceFile(path);
        }
    }

    model->restProjectInfo();
    model->waitModleUpdate();
    model->resetModle();
    return true;
}


/* 
    {
        "jsonrpc": "2.0",
        "method": "workspace/didRenameFiles",
        "params": {
            "files":[
                {
                    oldUri: string,
                    newUri: string,
                },
            ]
        }
    }
 */
bool DocumentRenameTask::handleNotification(const Json &req)
{
    auto model = QmlLanguageModel::Instance();
    auto project = ProjectExplorer::Project::Instance();

    bool bRest = false;
    for (auto & file : req["params"]["files"])
    {
        auto uri = file["oldUri"].get<std::string>();
        QUrl url = QUrl(OwO::Utf8ToQString(uri));
        if(url.scheme() == "file"){
            auto path = url.toLocalFile();
            project->removeSourceFile(path);
        }

        uri = file["newUri"].get<std::string>();
        url = QUrl(OwO::Utf8ToQString(uri));
        if(url.scheme() == "file"){
            auto path = url.toLocalFile();
            project->appendSourceFile(path);
        }
    }

    model->restProjectInfo();
    model->waitModleUpdate();
    model->resetModle();
    return true;
}

