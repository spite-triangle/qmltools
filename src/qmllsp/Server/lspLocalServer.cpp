
#include "lspLocalServer.h"

#include <QLocalSocket>


#include "common/lspLog.hpp"
#include "common/lspProject.h"

LspLocalServer::LspLocalServer(QObject * parent) 
    : LspServere(parent)
{
    m_server = new QLocalServer(this);
}

bool LspLocalServer::start() {
    auto project = ProjectExplorer::Project::Instance();

    if (m_server->listen(project->getSocketFile()) == false) {
        LOG_DEBUG("Failed to start local socket server.");
        return false;
    }

    if(waitClientConnect(m_server) == false){
        LOG_DEBUG("Failed to connect client.");
        return false;
    }

    run();

    return true;
}


