#ifndef LSPLOCALSERVER_H
#define LSPLOCALSERVER_H

#include <QLocalServer>

#include "Server/lspServer.h"

class LspLocalServer : public LspServere{
    Q_OBJECT
public:

    LspLocalServer(QObject * parent = nullptr);

    /* 启动服务 */
    virtual bool start() override;


private slots:

    /* socket 接收数据 */
    void onSocketResv();

    /* socket 关闭 */
    void onSocketClose();

private:
    QLocalServer * m_server;
};

#endif // LSPLOCALSERVER_H
