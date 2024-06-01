// git clone https://github.com/svenstaro/glsl-language-server.git


#include <string>
#include <iostream>
#include <fstream>

#include <QDir>
#include <QTextStream>
#include <QTcpSocket>
#include <QTcpServer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QCoreApplication>

#include "common/utils.h"
#include "common/lspLog.hpp"
#include "common/lspProject.h"
#include "server/lspLocalServer.h"

#include "handler/registerTask.h"



int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);

    auto project = ProjectExplorer::Project::Instance();

    // 解析参数
    int code = project->parserCommand(argc, argv);
    if(code != 0) return code;

    // 初始化日志
    OwO::Logger::Instance()->init( OwO::ToStdString(QDir(QCoreApplication::applicationDirPath() + "/log/qmllsp.log").absolutePath()), project->getExportLog());

    LspLocalServer server;
    RegisterTaskToServer(server);

    server.start();

    return app.exec();
}
