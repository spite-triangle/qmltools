#include "tokenFindTask.h"


#include "common/lspDefine.h"
#include "common/jsonUtil.hpp"
#include "common/jsonSerializer.hpp"
#include "qmlModel/qmlLanguageModel.h"
#include "qmlModel/qmljsfindreferences.h"

/* 

    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/references",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position":{
                "line": uint, "character": uint
            },
            "context": {
                "includeDeclaration": bool;
            }
        }
    }

    [
        {
            "uri": string,
            "range": range
        }
    ]

 */

bool ReferencesFindTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    auto pos = req["params"]["position"].get<POSITION_S>();

    QUrl url = QUrl(OwO::Utf8ToQString(uri));

    Json resJson;
    BLOCK{
        if(url.scheme() != "file") break;
        auto strPath = url.toLocalFile();
        if(strPath.endsWith(".qml", Qt::CaseInsensitive) == false) break;

        model->waitModleUpdate();
        if(model->isValid() == false) break;

        QmlJS::FindReferences finder;
        auto res = finder.findUsages(Utils::FilePath::fromString(strPath),pos);

        registerStop([&](){ res.cancel(); });
        res.waitForFinished();
        registerStop(Handler::StopFcn_t());

        resJson = finder.convertReference(res);
    }
    resp = JsonUtil::ResponseMessge(req, resJson);
    return true;
}

/* 

    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/definition",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position":{
                "line": uint, "character": uint
            }
        }
    }

    {
        "uri": string,
        "range": range
    }

 */

bool DefineTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    auto pos = req["params"]["position"].get<POSITION_S>();

    QUrl url = QUrl(OwO::Utf8ToQString(uri));

    Json resJson;
    BLOCK{
        if(url.scheme() != "file") break;
        auto strPath = url.toLocalFile();
        if(strPath.endsWith(".qml", Qt::CaseInsensitive) == false) break;

        model->waitModleUpdate();
        if(model->isValid() == false) break;

        QmlJS::FindReferences finder;
        resJson = finder.findDeclaration(Utils::FilePath::fromString(strPath),pos);
    }
    resp = JsonUtil::ResponseMessge(req, resJson);
    return true;
}
