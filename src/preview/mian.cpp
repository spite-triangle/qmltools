#include "test/testMain.hpp"

#include <QCoreApplication>

#include "previewTool.h"
#include "common/previewProject.h"
#include "debugClient/previewConnectManager.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

#ifdef DOCT_TEST
    TestMain(argc, argv);
#endif

    auto project =  ProjectExplorer::Project::Instance();


    project->setLanguage(QLocale());
    project->setFocusLocalQml("E:/workspace/qt servitor/tools/src/demo/main.qml");
    project->appendQrcFile("E:/workspace/qt servitor/tools/src/demo/main.qrc");
    project->setProjectFolder("E:/workspace/qt servitor/tools/src/demo/");

    PreviewTool* tool = new PreviewTool(&app);
    tool->init();
    tool->connectServer("socket:C:/Windows/Temp/preview_test/preview.socket");

    QObject::connect(&app, &QCoreApplication::aboutToQuit, tool, &PreviewTool::onApplicationQuit);

    return app.exec();
}

