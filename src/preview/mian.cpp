#include "test/testMain.hpp"

#include <QUrl>
#include <QDir>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>

#include "previewTool.h"
#include "common/utils.h"
#include "common/previewLog.hpp"
#include "common/previewProject.h"
#include "debugClient/previewConnectManager.h"

#include <csignal>

void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    qApp->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 接收 ctrl + c
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    auto project =  ProjectExplorer::Project::Instance();
    project->setRunFolder(QCoreApplication::applicationDirPath());

    // 解析参数
    int code = project->parserCommand(argc, argv);
    if(code != 0) return code;

    // 初始化日志
    OwO::Logger::Instance()->init( OwO::ToStdString(QDir(QCoreApplication::applicationDirPath() + "/log/preview.log").absolutePath()), 
                                project->getExportLog(), 
                                project->getConsoleLog());

#ifdef DOCT_TEST
    TestMain(argc, argv);
#endif


    // 工具
    PreviewTool* tool = new PreviewTool(&app);
    tool->init();

    // 连接调试器服务
    QTimer * timer = new QTimer(&app);
    timer->singleShot(100, [&](){
        QUrl server;
        if(project->getSocketFile().isEmpty()){
            server.setScheme("tcp");
            server.setHost(project->getHost());
            server.setPort(project->getPort());
        }else{
            server.setScheme("socket");
            server.setPath(project->getSocketFile());
        }
        tool->connectServer(server.toString());
    });
    

    /* 启动目标程序 */
    QProcess * target = nullptr;
    if(project->getTarget().isEmpty() == false){
        target = new QProcess(&app);
        auto env = QProcessEnvironment::systemEnvironment();
        target->setProcessEnvironment(env);
 
        // 程序启动成功，便激活 preview
        QObject::connect(target, &QProcess::stateChanged, [&](QProcess::ProcessState state) {
            if(state == QProcess::Running){
                timer->start();
            }
        });

        QObject::connect(target, &QProcess::finished, [](int exitCode, QProcess::ExitStatus exitStatus){
            LOG_DEBUG("sub process is closed.");
            QCoreApplication::quit();
        });

        QObject::connect(target, &QProcess::errorOccurred, [&](QProcess::ProcessError error){
            LOG_DEBUG("sub process crash.");
            QCoreApplication::quit();
        });

        // 启动子程序
        QString server;
        if(project->getSocketFile().isEmpty()){
            server = QString("host:%1,port:%2").arg(project->getHost()).arg(QString::number(project->getPort()));
        }else{
            server = QString("file:%1").arg(project->getSocketFile());
        }

        QStringList lstArgs;
        lstArgs.append(QString("-qmljsdebugger=%1,block").arg(server));
        target->start(project->getTarget(), lstArgs);
    }else{
        timer->start();
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        tool->onApplicationQuit();

        // 关闭子进程
        if(target != nullptr){
            target->kill();
            target->waitForFinished();
            target = nullptr;
        }
    });

    return app.exec();
}

