#ifndef LSPLOCALSERVER_H
#define LSPLOCALSERVER_H

#include <QLocalServer>

#include "Server/lspServer.h"

class LspLocalServer : public LspServere{
    Q_OBJECT
public:

    LspLocalServer(QObject * parent = nullptr);

    virtual bool start() override;



private:
    QLocalServer * m_server;
};

#endif // LSPLOCALSERVER_H
