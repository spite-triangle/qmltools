
#include "initializeTask.h"

#include "common/jsonUtil.hpp"

#include "qmlModel/qmlLanguageModel.h"

bool InitializeHandler::handleMessage(const Json & req, Json & resp) {

    Json text_document_sync{
        { "openClose", true }, // 是否发送文件打开和关闭的信号
        { "change", 1 }, // 文件修改时，客户端通知服务。主要用于语法检测
        { "willSave", false },
        { "willSaveWaitUntil", false },
        { "save", { { "includeText", false } } },
    };

    Json completion_provider{
        { "resolveProvider", false },
        {"triggerCharacters", Json::array()}
    };

    Json result{
        {
            "capabilities",{
                { "textDocumentSync", text_document_sync }, // 文件被修改时，如何通知服务
                // { "completionProvider", completion_provider }, // 代码补全
                { "hoverProvider", true }, // 悬停提示
                // { "definitionProvider", true }, // 跳转到定义
                // { "referencesProvider", false }, // 哪些地方引用
                // { "documentFormattingProvider", true }, // 格式化
                // { "documentRangeFormattingProvider", true },
                // { "documentSymbolProvider", false }, // 在 vscode 中使用 @ 查找符号
                // { "colorProvider", false}, // 拾色器
                // { "renameProvider", true}, 
                { "experimental", {} }, 
            }
        }
    };

    resp = JsonUtil::ResponseMessge(req, result);
    return true;
}


bool InitializedHandler::handleNotification(const Json &req)
{
   return QmlLanguageModel::Instance()->updateProjectInfo();
}
