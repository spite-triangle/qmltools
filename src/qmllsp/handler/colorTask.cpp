#include "colorTask.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/colorPresentation",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            "position":{
                "line": uint, "character": uint
            }
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

    return false;
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

    return false;
}
