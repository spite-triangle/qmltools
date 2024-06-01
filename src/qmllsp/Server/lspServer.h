#ifndef LSPSERVER_H
#define LSPSERVER_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QFuture>
#include <QIODevice>
#include <QByteArray>

#include "handleInterface/Task.h"
#include "common/lspDefine.h"
#include "common/SafeQueue.hpp"

class LspServere : public QObject{
    Q_OBJECT

public:
    struct LSP_MESSAGE_S{
        qsizetype nLen = -1;    // Content-Length
        QString strType;    // Content-Type
        QByteArray content;    // jsonrpc
    };

public:
    LspServere(QObject * parent = nullptr)
        : QObject(parent)
    {}

    virtual bool start() = 0;
    void registoryTaskFactory(const QString & strMethod, const TaskFactory::Ptr & factory);

protected:
    template<class Server>
    bool waitClientConnect(Server * pServer){
        if(pServer->waitForNewConnection(60000) == false) return false; 

        m_pSocket = pServer->nextPendingConnection();
        if(m_pSocket == nullptr) return false;
        return true;
    };

    void run();

private:
    void runSocketResvRequest();
    void runSocketSendResponse();

    bool recv(LSP_MESSAGE_S & stMsg);
    bool recvHead(LSP_MESSAGE_S & stMsg);
    void send(const QByteArray & data);
    qsizetype findIndexFromSocket(const QByteArray & target);

    bool distributeTask(const JsonObjectPtr & json);
    bool handlerPostCallback(const QString & id, bool bRes);

    /* 任务管理 */
    void storeTask(const QString & id, const Task::Ptr & task);
    void removeTask(const QString & id);
    Task::Ptr findTask(const QString & id);
private:
    QIODevice * m_pSocket; // 服务只连接一个 socket

    QFuture<void> m_resResv; // 用于等待 resv 循环结束
    QFuture<void> m_resSend; // 用于等待 send 循环结束

    QMap<QString , TaskFactory::Ptr> m_mapTaskFactory; // 处理任务创建工厂

    SafeQueue<QString> m_queueResps; // 存放 task 的 id

    QMutex m_mutTask; // m_mapTasks 的保护锁
    QMap<QString, Task::Ptr> m_mapTasks; // map<id, request task>
};


#endif // LSPSERVER_H
