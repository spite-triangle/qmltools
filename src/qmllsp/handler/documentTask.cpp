#include "documentTask.h"

#include <QUrl>

#include "common/utils.h"
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
        model->updateSourceFile(url.toLocalFile());
    }
    return true;
}
