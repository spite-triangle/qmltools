#include "colorTask.h"

#include <iomanip>

#include <QUrl>
#include <QString>

#include "common/utils.h"
#include "common/jsonUtil.hpp"
#include "qmlModel/qmlLanguageModel.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/colorPresentation",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            color: Color;
            range: Range;
        }
    }

    {
        label: string;
        textEdit?: TextEdit;
        additionalTextEdits?: TextEdit[];
    }
 */

bool ColorPresentationTask::handleMessage(const Json &req, Json &resp)
{
    auto color = req["params"]["color"];
    RANGE_S range = req["params"]["range"].get<RANGE_S>();

    int r = color["red"].get<double>() * 255;
    int g = color["green"].get<double>() * 255;
    int b = color["blue"].get<double>() * 255;
    int a = color["alpha"].get<double>() * 255;

    std::string hex = OwO::Format( "#",
                            std::setw(2), std::setfill('0'),std::hex, a, 
                            std::setw(2), std::setfill('0'),std::hex, r,
                            std::setw(2), std::setfill('0'),std::hex, g, 
                            std::setw(2), std::setfill('0'),std::hex, b);

    Json res{
        {"label", hex},
        {"textEdit",{
            {"range", range},
            {"newText", hex}
        }}
    };

    checkInterrupt();

    resp = JsonUtil::ResponseMessge(req, Json::array({res}));
    return true;
}


bool ColorPresentationTask::handleInterrupt(const Json & req, Json & resp)
{
    Json error{
        {"code", LSP_ERROR_E::REQUEST_CANCELLED},
        {"message", "cancel complete"}
    };

    resp = JsonUtil::ResponseError(req, error);
    return true;
}



/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/documentColor",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            partialResultToken?: ProgressToken;
            workDoneToken?: ProgressToken;
        }
    }

    [
        {
            range : Range;
            color : Color;
        }
    ]
 */

bool DocumentColorTask::handleMessage(const Json &req, Json &resp)
{
    auto model = QmlLanguageModel::Instance();
    auto uri = req["params"]["textDocument"]["uri"].get<std::string>();
    QUrl url = QUrl(OwO::Utf8ToQString(uri));
    auto strPath = url.toLocalFile();

    Json colors = Json::array();
    if(url.scheme() == "file" && strPath.endsWith(".qml", Qt::CaseInsensitive)){
        colors = model->queryColor(strPath);
    }
    resp = JsonUtil::ResponseMessge(req, colors); 
    return true;
}
