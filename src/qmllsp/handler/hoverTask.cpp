#include "hoverTask.h"

#include "common/JsonUtil.hpp"


/* 
    {
        "jsonrpc": "2.0",
        "method": "textDocument/hover",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position": {
                "line": uint,
                "character" : uint,
            }
            "workDoneToken": ? 
        }
    }
 */

bool HoverTask::handleMessage(const Json & req, Json & resp)
{
    auto uri = req["params"]["textDocument"]["uri"];
    auto position = req["params"]["position"];
    int line = position["line"];
    int character = position["character"];


    Json test{
        {"contents", { 
            { "language", "qml" }, 
            { "value", "atesetfasfas"} 
        }}
    };

    resp = JsonUtil::ResponseMessge(req, test);
    return true;
}