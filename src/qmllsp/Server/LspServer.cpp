
#include "lspServer.h"


#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>

#include <string>
#include <stdexcept>

#include "common/lspLog.hpp"
#include "common/lspProject.h"
#include "common/lspException.hpp"
#include "common/jsonUtil.hpp"


namespace Internal
{

// header 与 content 的边界
const char* g_boundaryLine = "\r\n\r\n";

const char* g_lineEnd = "\r\n";

} // namespace Internal


LspServer::Ptr LspServer::createServer()
{
    return Ptr( new LspServer());
}

bool LspServer::distributeTask(const JsonPtr & req)
{

    auto & json = *req;
    QString method = OwO::Utf8ToQString(json["method"].get<std::string>());

    // 区分 message 
    if(json.contains("id") == true){
        QString id = OwO::ConvertToQString(json["id"].get<int>());

        if(m_mapTaskFactory.contains(method) == false) return true;

        // 创建
        Task::Ptr task = m_mapTaskFactory[method]->createMessageTask(id, method);
        if(task == nullptr) return false;

        task->getHandler()->setServer(shared_from_this());

        // 发布
        task->distribute(req,[this](const QString & id, bool bRes){
            return handlerPostCallback(id,bRes);
        });

        // 保存任务
        storeTask(id, task);
    }else{
        if(m_mapTaskFactory.contains(method) == false) return true;

        // 创建
        Task::Ptr task = m_mapTaskFactory[method]->createNotificationTask(method);
        if(task == nullptr) return false;

        task->getHandler()->setServer(shared_from_this());

        // 发布
        task->distribute(req);
    }
    return true;
}

bool LspServer::handlerPostCallback(const QString &id, bool bRes)
{
    // 任务结束
    if(bRes == true){
        auto task = dynamic_cast<TaskMessage*>(findTask(id).get());
        if(task == nullptr) return false;

        RESPONSE_MESSAGE_S msg;
        msg.msgbody = task->getResponsePtr();
        msg.id = id;

        m_queueResps.waitPush(msg);
    }
    return bRes;
}



bool LspServer::start()
{
    auto project = ProjectExplorer::Project::Instance();

    ASSERT_RETURN(tcp::InitSocketNet() == true, "Failed to init net.", false);

    m_serverSocket = tcp::InitServer("127.0.0.1", project->getPort());
    ASSERT_RETURN(m_serverSocket != -1, "Failed to init server socket.", false);

    // 等 10s
    m_connectSocket = tcp::AcceptClientTimeOut(m_serverSocket, 10000);
    ASSERT_RETURN(m_connectSocket.fd != -1, "Failed to connect client.", false);


    m_resResv = QtConcurrent::run(&LspServer::runSocketResvRequest, this);
    m_resSend = QtConcurrent::run(&LspServer::runSocketSendResponse, this);
    return true;
}

bool LspServer::close()
{
    tcp::CloseSocket(m_connectSocket.fd);
    tcp::CloseSocket(m_serverSocket);
    return tcp::CloseSocketNet();
}

void LspServer::registoryTaskFactory(const QString &strMethod, const TaskFactory::Ptr &factory)
{
    m_mapTaskFactory[strMethod] = factory;
}

void LspServer::sendMsg(const JsonPtr &msgBody)
{
    RESPONSE_MESSAGE_S msg;
    msg.msgbody = msgBody;

    m_queueResps.waitPush(msg);
}

void LspServer::runSocketResvRequest()
try
{
    LSP_MESSAGE_S stMsg;
    while (true)
    {
        // 接收信息，只有 content 有内容才做处理
        if(recv(stMsg) == false){
            throw ParseRequestException("Failed to recv.");
        }

        /* 
            {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "textDocument/completion",
                "params": {
                    ...
                }
            }
         */
        JsonPtr req  = std::make_shared<Json>(Json::parse(stMsg.content, nullptr, false));
        if(req == nullptr || req->is_null()){
             throw ParseRequestException("Failed to load json.");
        }

        // 根据方法创建任务，并存入 m_maptasks
        if(distributeTask(req) == false){
             throw LspException("Failed to distribute task.");
        }

        // 重置
        stMsg = LSP_MESSAGE_S();
    }
}
catch (const std::exception & error){
    LOG_ERROR("%s",error.what());

    auto msg = std::make_shared<Json>(R"({"jsonrpc":"2.0","method":"exit"})");
    sendMsg(msg);

    m_queueResps.pushNull();
}

void LspServer::runSocketSendResponse()
{
    while (true)
    {
        auto elem = m_queueResps.waitPop();
        if(elem == nullptr) return;

        if(elem->id.isEmpty() == false){
            removeTask(elem->id);
        }

        auto msg = genarateSendMessage(elem->msgbody);
        send(msg);
    }
}

std::string LspServer::genarateSendMessage(const JsonPtr &json)
{
    if(json == nullptr) return std::string();

 
    auto content = json->dump(4);

    std::string header;
    header.append("Content-Length: " + std::to_string(content.size()));
    header.append(Internal::g_lineEnd);
    header.append("Content-Type: application/vscode-jsonrpc;charset=utf-8");
    header.append(Internal::g_boundaryLine);

    return header + content;
}


bool LspServer::recv(LSP_MESSAGE_S &stMsg)
{
    // 解析头
    ASSERT_RETURN(recvHead(stMsg) == true, "Failed to recv head information.", false);

    // 获取内容
    char * buff = new char[stMsg.nLen + 1]();
    ASSERT_RETURN(tcp::RecvMsg(m_connectSocket.fd, buff, stMsg.nLen) == true, "Failed to recv content.", false)

    stMsg.content = OwO::QStringToUtf8(buff);
    return true;
}

bool LspServer::recvHead(LSP_MESSAGE_S &stMsg)
{
    if(stMsg.nLen >= 0)  return true;

    // 查找 header 与 content 的分界线
    std::string strHead;
    if(tcp::RecvMsg(m_connectSocket.fd, Internal::g_boundaryLine, strHead) == false){
        return false;
    }

    // 解析头
    QStringList lstHead =  OwO::Utf8ToQString(strHead).split(Internal::g_lineEnd);
    if(lstHead.size() <= 0){
        throw ParseRequestException("Not found valid header information.");
    }

    for (auto line : lstHead)
    {
        if(line.startsWith("Content-Length")){
            bool bOk;
            stMsg.nLen = line.mid(15).trimmed().toInt(& bOk);
            if(bOk == false || stMsg.nLen < 0){
                throw ParseRequestException("Failed to parse content length.");
            }
        }

        if(line.startsWith("Content-Type")){
            stMsg.strType = OwO::QStringToUtf8(line.mid(13).trimmed());
        }
    }
    return true;
}

bool LspServer::send(const std::string  &data)
{
    const auto & total = data.length();

    size_t sendLen = 0;
    int len = 1024; // 一次发送的长度

    while (sendLen < total)
    {
        size_t diff = total - sendLen;
        len =  diff < len ?  diff : len;
        
        int res = tcp::SendMsg(m_connectSocket.fd, data.c_str() + sendLen, len);
        if(res < 0 ) return false;

        sendLen += res;
    }

    return true;
}



void LspServer::storeTask(const QString & id, const Task::Ptr & task) {
    QMutexLocker lock(&m_mutTask);
    m_mapTasks.insert(id, task);
}

void LspServer::removeTask(const QString & id) {
    QMutexLocker lock(&m_mutTask);
    if(m_mapTasks.contains(id) == false) return;

    m_mapTasks.remove(id);
}

Task::Ptr LspServer::findTask(const QString &id)
{
    QMutexLocker lock(&m_mutTask);
    if(m_mapTasks.contains(id) == false) return Task::Ptr(nullptr);
    return m_mapTasks[id];
}


void LspServer::onDiagnosticMessageUpdated(const JsonPtr & param) {
    ASSERT_RETURN(param != nullptr && param->is_null() == false, "diagnostic is null");

    auto msg = JsonUtil::NotificationMessage("textDocument/publishDiagnostics", *param);
    sendMsg(std::make_shared<Json>(std::move(msg)));
}