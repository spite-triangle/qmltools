
#include "initializeTask.h"

#include "common/jsonUtil.hpp"

#include "qmlModel/qmlLanguageModel.h"

bool InitializeHandler::handleMessage(const Json &req, Json &resp)
{

    Json text_document_sync{
        {"openClose", true}, // 是否发送文件打开和关闭的信号
        {"change", 1},       // 文件修改时，客户端通知服务。主要用于语法检测
        {"willSave", false},
        {"willSaveWaitUntil", false},
        {"save", {{"includeText", false}}},
    };

    Json completion_provider{
        {"resolveProvider", false},
        {"triggerCharacters", Json::array()}};

    Json filters = Json::array({{{"scheme", "file"},
                                 {"matches", "file"},
                                 {"pattern", {{"glob", "**/*.{qml,js}"}}}},
                                {{"scheme", "file"},
                                 {"matches", "file"},
                                 {"pattern", {{"glob", "**/qmldir"}}}}});

    Json workspace{
        {"fileOperations", {
                               {"didCreate", {{"filters", filters}}},
                               {"didRename", {{"filters", filters}}},
                               {"didDelete", {{"filters", filters}}},
                           }}};

    Json result{
        {"capabilities", {
                             {"textDocumentSync", text_document_sync}, // 文件被修改时，如何通知服务
                             {"workspace", workspace},
                             {"completionProvider", completion_provider}, // 代码补全
                             {"hoverProvider", false},                    // 悬停提示
                             {"definitionProvider", true},                // 跳转到定义
                             {"referencesProvider", true},                // 哪些地方引用
                             {"documentFormattingProvider", true},        // 格式化
                             // { "documentRangeFormattingProvider", true },
                             // { "documentSymbolProvider", false }, // 在 vscode 中使用 @ 查找符号
                             {"colorProvider", true}, // 拾色器
                             {"renameProvider", true},
                             {"experimental", {}},
                         }}};

    resp = JsonUtil::ResponseMessge(req, result);
    return true;
}

bool InitializedHandler::handleNotification(const Json &req)
{
    QmlLanguageModel::Instance()->restProjectInfo();
    QmlLanguageModel::Instance()->waitModleUpdate();
    QmlLanguageModel::Instance()->updateSourceFile();


    /* NOTE - 初始运行，必须有，不知道为啥 */
    auto model = QmlLanguageModel::Instance();
    model->openFile("qrc:/demo.qml", 0);
    model->restProjectInfo();
    model->waitModleUpdate();
    model->resetModle();
    model->setCurrFocusFile("qrc:/demo.qml");
    model->updateSourceFile("qrc:/demo.qml");
    return true;
}
