
#include "lspServer.h"


#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>

#include <string>
#include <stdexcept>

#include "common/lspLog.hpp"
#include "common/lspException.hpp"
#include "common/jsonUtils.hpp"

#include "handler/initializeTask.h"

namespace Internal
{

// header 与 content 的边界
const QByteArray g_boundaryLine = "\r\n\r\n";

const QByteArray g_lineEnd = "\r\n";

} // namespace Internal


bool LspServere::distributeTask(const JsonObjectPtr & req)
{
    JsonUtils utils(req);
    QString id = utils.valueException("id").toString();
    QString method = utils.valueException("method").toString();

    // 创建
    Task::Ptr task;
    if(method == "initialize"){
        task = Task::makeTask(id, method, std::make_shared<InitializeHandler>());
    }

    // 发布
    if(task == nullptr) return false;
    task->distribute(req,[this](const QString & id, bool bRes){
        // 任务结束
        if(bRes == true){
            m_queueResps.waitPush(id);
        }
        return bRes;
    });

    // 保存任务
    storeTask(id, task);
    return true;
}


void LspServere::run()
{
    m_resResv = QtConcurrent::run(&LspServere::runSocketResv, this);
    m_resSend = QtConcurrent::run(&LspServere::runSocketSend, this);
}


void LspServere::runSocketResv()
try
{
    LSP_MESSAGE_S stMsg;
    while (true)
    {
        if(m_pSocket == nullptr) return;
        if(m_pSocket->waitForReadyRead(-1) == false) return;

        // 接收信息，只有 content 有内容才做处理
        if(recv(stMsg) == false) continue;

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
        JsonObjectPtr req;
        JsonUtils utils(req);
        if(utils.load(stMsg.content) == false){
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
catch (const LspException & error){
    LOG_ERROR("%s",error.what());
    m_queueResps.waitPush(R"({"jsonrpc":"2.0","method":"exit"})");
    m_queueResps.pushNull();
}

void LspServere::runSocketSend()
{
    while (true)
    {
        auto elem = m_queueResps.waitPop();
        if(elem == nullptr) return;

        // 根据 id 获取任务
        auto id = *elem;
        auto task = findTask(id);


        task->getResponsePtr();


        removeTask(id);

        // QByteArray resp = QString("Content-Length: %1%2%3")
        //                             .arg(msg->size())
        //                             .arg(Internal::g_boundaryLine)
        //                             .arg(*msg).toLocal8Bit();
        // send(resp);
    }
}

bool LspServere::recv(LSP_MESSAGE_S &stMsg)
{
    if(recvHead(stMsg) == false) return false;

    // 读取内容
    if(m_pSocket->bytesAvailable() >= stMsg.nLen){
        stMsg.content = m_pSocket->read(stMsg.nLen);
        return true;
    }

    return false;
}

bool LspServere::recvHead(LSP_MESSAGE_S &stMsg)
{
    if(stMsg.nLen >= 0)  return true;

    // 查找 header 与 content 的分界线
    qsizetype index = findIndexFromSocket(Internal::g_boundaryLine);
    if( index < 0) return false;

    // 从 socket 中读取 header
    auto header = m_pSocket->read(index + Internal::g_boundaryLine.size());
    QStringList lstHead = QString(header).split(Internal::g_lineEnd);
    if(lstHead.size() <= 0){
        throw ParseRequestException("Not found valid header information.");
    }

    // 解析头
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
            stMsg.strType = line.mid(13).trimmed();
        }
    }

    return true;
}

void LspServere::send(const QByteArray &data)
{
    const auto & total = data.length();

    size_t sendLen = 0;
    int len = 1024; // 一次发送的长度

    while (sendLen < total)
    {
        size_t diff = total - sendLen;
        len =  diff < len ?  diff : len;
        sendLen += m_pSocket->write(data.data() + sendLen, len);
    }
}


qsizetype LspServere::findIndexFromSocket(const QByteArray &target)
{
    m_pSocket->startTransaction();
    auto bytes = m_pSocket->readAll();
    auto index = bytes.indexOf(target);
    m_pSocket->rollbackTransaction();
    return index;
}


void LspServere::storeTask(const QString & id, const Task::Ptr & task) {
    QMutexLocker lock(&m_mutTask);
    m_maptasks.insert(id, task);
}

void LspServere::removeTask(const QString & id) {
    QMutexLocker lock(&m_mutTask);
    if(m_maptasks.contains(id) == false) return;

    m_maptasks.remove(id);
}

const Task::Ptr & LspServere::findTask(const QString &id)
{
    QMutexLocker lock(&m_mutTask);
    if(m_maptasks.contains(id) == false) return;
    return m_maptasks[id];
}
