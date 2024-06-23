#ifndef LSPSERVER_H
#define LSPSERVER_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QFuture>

#include <memory>

#include "server/Socket.h"

#include "common/lspDefine.h"
#include "common/SafeQueue.hpp"
#include "handleInterface/Task.h"

class LspServer : public QObject, public std::enable_shared_from_this<LspServer>{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<LspServer>;

    struct LSP_MESSAGE_S{
        int64_t nLen = -1;    // Content-Length
        std::string strType;    // Content-Type
        std::string content;    // jsonrpc
    };

    struct RESPONSE_MESSAGE_S{
        QString id;     // 存在值，表示 response； 空，表示服务主动发送消息
        JsonPtr msgbody;
    };

public:
    static Ptr createServer();

    /* 启动服务 */
    bool start();

    /* 关闭服务 */
    bool close();

    /* 注册处理器 */
    void registoryTaskFactory(const QString & strMethod, const TaskFactory::Ptr & factory);


    /* 发送信息 */
    void sendMsg(const JsonPtr & msgBody);

public slots:

    void onDiagnosticMessageUpdated(const JsonPtr & param);

protected:
    LspServer(QObject * parent = nullptr)
        : QObject(parent)
    {}

private:
    void runSocketResvRequest();
    void runSocketSendResponse();

    bool recv(LSP_MESSAGE_S & stMsg);
    bool recvHead(LSP_MESSAGE_S & stMsg);
    bool send(const std::string & data);

    bool distributeTask(const JsonPtr & json);
    bool handlerPostCallback(const QString & id, bool bRes);

    std::string genarateSendMessage(const JsonPtr & json);

    /* 任务管理 */
    void storeTask(const QString & id, const Task::Ptr & task);
    void removeTask(const QString & id);
    Task::Ptr findTask(const QString & id);
private:
    tcp::socket_t m_serverSocket; // 服务
    tcp::SOCKET_INFO_S m_connectSocket; // 连接 socket

    QFuture<void> m_resResv; // 用于等待 resv 循环结束
    QFuture<void> m_resSend; // 用于等待 send 循环结束

    QMap<QString , TaskFactory::Ptr> m_mapTaskFactory; // 处理任务创建工厂

    SafeQueue<RESPONSE_MESSAGE_S> m_queueResps; // 存放需要发送的信息

    QMutex m_mutTask; // m_mapTasks 的保护锁
    QMap<QString, Task::Ptr> m_mapTasks; // map<id, request task>
};


#endif // LSPSERVER_H
