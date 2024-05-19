
#include "lspLocalServer.h"

#include <QLocalSocket>
#include <QCoreApplication>

#include "common/lspLog.hpp"
#include "common/lspProject.h"

LspLocalServer::LspLocalServer(QObject * parent) 
    : LspServere(parent)
{
    m_server = new QLocalServer(this);

    connect(m_server, &QLocalServer::newConnection, [=]() {
        QLocalSocket* socket = m_server->nextPendingConnection();
        
        // socket 接收数据
        // 请求数据量太大时，可能触发多次
        connect(socket, &QLocalSocket::readyRead, this, & LspLocalServer::onSocketResv);

        // 客户端断开
        connect(socket, &QLocalSocket::aboutToClose, this, & LspLocalServer::onSocketClose);
    });
}

bool LspLocalServer::start() {
    auto project = ProjectExplorer::Project::Instance();

    if (m_server->listen(project->getSocketFile()) == false) {
        LOG_DEBUG("Failed to start local socket server.");
        return false;
    }

    return true;
}


void LspLocalServer::onSocketResv() {
    QLocalSocket * socket = qobject_cast<QLocalSocket*>(sender());
    if(socket == nullptr) return;

    auto datas = recv(socket);
    if(datas.isEmpty()) return;

    // TODO -  接收数据处理
}


void LspLocalServer::onSocketClose()
{
    // REVIEW - lsp 只服务一个客户端，直接关闭服务
    QCoreApplication::exit(0);
}
