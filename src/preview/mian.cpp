#include "test/testMain.hpp"

#include <QTimer>
#include <QProcess>
#include <QCoreApplication>

#include "previewTool.h"
#include "common/previewProject.h"
#include "debugClient/previewConnectManager.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    auto project =  ProjectExplorer::Project::Instance();
    if(project->parserCommand(argc, argv) == false) return -1;

#ifdef DOCT_TEST
    project->setLanguage(QLocale());
    project->setFocusLocalQml("E:/workspace/qt_servitor/tools/src/demo/main.qml");
    project->appendQrcFile("E:/workspace/qt_servitor/tools/src/demo/main.qrc");
    project->setProjectFolder("E:/workspace/qt_servitor/tools/src/demo/");
    TestMain(argc, argv);
#endif

    PreviewTool* tool = new PreviewTool(&app);
    tool->init();

    // 连接调试器服务
    QTimer * timer = new QTimer(&app);
    timer->singleShot(100, [&](){
        QString server;
        if(project->getSocketFile().isEmpty()){
            server = QString("tcp:%1:%2").arg(project->getHost()).arg(QString::number(project->getPort()));
        }else{
            server = QString("socket:%1").arg(project->getSocketFile());
        }
        tool->connectServer(server);
    });
    

    /* 启动目标程序 */
    QProcess * target = nullptr;
    if(project->getTarget().isEmpty() == false){
        target = new QProcess(&app);
        target->setEnvironment(QProcess::systemEnvironment());
 
        // 程序启动成功，便激活 preview
        QObject::connect(target, &QProcess::stateChanged, [&](QProcess::ProcessState state) {
            if(state == QProcess::Running){
                timer->start();
            }
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

