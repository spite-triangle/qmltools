#include "renameTask.h"

#include "common/lspDefine.h"
#include "common/jsonUtil.hpp"
#include "common/jsonSerializer.hpp"
#include "qmlModel/qmlLanguageModel.h"
#include "qmlModel/qmljsfindreferences.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/rename",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position":{
                "line": uint, "character": uint
            },
            newName: string;
        }
    }

    // WorkspaceEdit 
    {
        changes?:{
            "uri":[
                {
                    range: Range,
                    newText:string
                }
            ] 
        },
        changeAnnotations?:
    }

 */
bool RenameTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();

    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    auto pos = req["params"]["position"].get<POSITION_S>();
    auto newName = req["params"]["newName"].get<std::string>();

    QUrl url = QUrl(OwO::Utf8ToQString(uri));

    Json resJson;
    BLOCK{
        if(url.scheme() != "file") break;
        auto strPath = url.toLocalFile();
        if(strPath.endsWith(".qml", Qt::CaseInsensitive) == false) break;

        model->waitModleUpdate();
        if(model->isValid() == false) break;

        QmlJS::FindReferences finder;
        auto res = finder.renameUsages(Utils::FilePath::fromString(strPath), OwO::Utf8ToQString(newName) ,pos);

        registerStop([&](){ res.cancel(); });
        res.waitForFinished();
        registerStop(Handler::StopFcn_t());

        auto usageJson = finder.convertRename(res);
        if(usageJson.is_null()) break;
        resJson = Json{
            {"changes", usageJson}
        };
    }

    resp = JsonUtil::ResponseMessge(req, resJson);
    return true;
}