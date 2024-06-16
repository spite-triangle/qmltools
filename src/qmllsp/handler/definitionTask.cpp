#include "definitionTask.h"

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
            "workDoneToken?": 
            "partialResultToken?":
        }
    }

    [
        {
            uri: string,
            range:{
                start: Position;
                end: Position;
            }
        }
    ] 

 */
bool DefinitionTask::handleMessage(const Json &req, Json &resp)
{
    return false;
}