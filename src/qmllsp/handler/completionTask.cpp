#include "completionTask.h"


#include <QUrl>

#include "utils/filepath.h"

#include "common/jsonUtil.hpp"
#include "common/jsonSerializer.hpp"
#include "qmlModel/qmlCompletion.h"
#include "qmlModel/qmlLanguageModel.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/completion",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position":{
                "line": uint, "character": uint
            }
            "workDoneToken?": 
            "partialResultToken?":
            "context?":{
                "triggerKind":
                "triggerCharacter?" : string
            }
        }
    }

    {
        "isIncomplete": boolean,
        "itemDefaults?":{},
        "items"[
            {
                "label": string,
                detail?: string,
                kind?: CompletionItemKind
            }
        ]
    }|[
        {
            "label": string,
            detail?: string,
            kind?: CompletionItemKind
        }
    ]

 */
bool CompletionTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));
    QString path = url.toLocalFile();

    Json completions;
    if(url.scheme() == "file" && path.endsWith(".qml", Qt::CaseInsensitive)){
        auto currFile = Utils::FilePath::fromString(model->getCurrFocusFile());
        auto targetFile = Utils::FilePath::fromString(path);
        model->waitModleUpdate();

        if(currFile != targetFile){
            model->setCurrFocusFile(path);
            model->updateSourceFile(path);
        }
        model->waitModelManagerUpdate();

        POSITION_S pos = req["params"]["position"];

        QmlCompletion tool;
        tool.setSemanticInfo(model->getCurrentSemantic());
        tool.setPosition(pos);
        tool.setCheckPoint([&](){
            return checkInterrupt();
        });

        completions = tool.complete();
    }else{
        completions = Json::array();
    }

    resp = JsonUtil::ResponseMessge(req, completions);
    return true;
}

bool CompletionTask::handleInterrupt(const Json & req, Json & resp)
{
    Json error{
        {"code", LSP_ERROR_E::REQUEST_CANCELLED},
        {"message", "cancel complete"}
    };

    resp = JsonUtil::ResponseError(req, error);
    return true;
}
