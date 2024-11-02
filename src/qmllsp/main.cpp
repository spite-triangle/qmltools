// git clone https://github.com/svenstaro/glsl-language-server.git


#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

#include <QDir>
#include <QTextStream>
#include <QTcpSocket>
#include <QTcpServer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QCoreApplication>

#include "common/utils.h"
#include "common/lspLog.h"
#include "common/lspProject.h"
#include "server/lspServer.h"
#include "handler/registerTask.h"
#include "qmlModel/qmlLanguageModel.h"
#include "utils/mimeutils.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    auto project = ProjectExplorer::Project::Instance();
    

    // 解析参数
    int code = project->parserCommand(argc, argv);
    if(code != 0) return code;

    // 初始化日志
    auto begin =  std::chrono::steady_clock::now();
    OwO::Logger::Instance()->init( OwO::ToStdString(QDir(QCoreApplication::applicationDirPath() + "/log/qmllsp.log").absolutePath()), project->getExportLog());
    auto end =  std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;

    auto server = LspServer::createServer();
    RegisterTaskToServer(server);
    server->start();

    Utils::setMimeStartupPhase(Utils::MimeStartupPhase::PluginsLoading);
    Utils::setMimeStartupPhase(Utils::MimeStartupPhase::PluginsInitializing);
    Utils::setMimeStartupPhase(Utils::MimeStartupPhase::PluginsDelayedInitializing);
    Utils::setMimeStartupPhase(Utils::MimeStartupPhase::UpAndRunning);

    // 创建语言模型
    auto model = QmlLanguageModel::Instance();
    QObject::connect(model.get(), &QmlLanguageModel::sigDiagnosticMessageUpdated, server.get(), &LspServer::onDiagnosticMessageUpdated); 
    return app.exec();
}
