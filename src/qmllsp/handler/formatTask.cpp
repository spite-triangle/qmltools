#include "formatTask.h"

#include <QUrl>

#include "common/utils.h"
#include "common/jsonUtil.hpp"
#include "qmlModel/qmlLanguageModel.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/formatting",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            options:{
                tabSize: uinteger;
                insertSpaces: boolean;
                [key: string]: boolean | integer | string;
            }

        }
    }

    [
        {
            range: Range;
            newText: string;
        }
    ]
 */
bool FormatTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();
    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));
    auto strPath = url.toLocalFile();

    Json text;
    if(url.scheme() == "file" && strPath.endsWith(".qml", Qt::CaseInsensitive)){
        uint32_t uTableSize = req["params"]["options"]["tabSize"].get<uint32_t>();
        text = model->formatFile(strPath, uTableSize);
    }

    resp = JsonUtil::ResponseMessge(req, text);
    return true;
}


