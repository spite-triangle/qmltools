#include "renameTask.h"

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/hover",
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
            "uri": {
                range: Range,
                newText:string
            }
        },
        documentChanges?: 
        changeAnnotations?:
    }

 */
bool RenameTask::handleMessage(const Json &req, Json &resp)
{

    return false;
}