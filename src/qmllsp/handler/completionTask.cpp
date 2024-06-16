#include "completionTask.h"



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

    return false;
}