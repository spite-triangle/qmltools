#include "formatTask.h"

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

    {
        range: Range;
        newText: string;
    }
 */
bool FormatTask::handleMessage(const Json &req, Json &resp)
{
    return false;
}

/* 
    {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "textDocument/rangeFormatting",
        "params": {
            "textDocument":{
                "uri" : string,
            },
            options:{
                tabSize: uinteger;
                insertSpaces: boolean;
                [key: string]: boolean | integer | string;
            },
            range: Range;

        }
    }

    [
        {
            range: Range;
            newText: string;
        }
    ]
 */
bool RangeFormatTask::handleMessage(const Json &req, Json &resp)
{
    return false;
}
