#ifndef PREVIEWTOOL_H
#define PREVIEWTOOL_H

#include <thread>
#include <memory>

#include <QObject>

#include "common/command.h"
#include "FileSystemManager.h"
#include "debugClient/previewConnectManager.h"

class PreviewTool : public QObject{
    Q_OBJECT
public:
    PreviewTool(QObject * parent = nullptr);

    bool init();
    void connectServer(const QString & strServer);

signals:
    void sigFocusChange(const QString & path);
    void sigRerun();
    void sigZoom(float zoomFactor);

public slots:
    void onDebugServiceUnavailable(const QString & name);
    void onApplicationQuit();

private:
    void initCommands();
    void runInterface();
    static void closeThread(std::thread * p);

private:
    static bool m_bQuit;
    std::shared_ptr<std::thread> m_pInterface;
    OwO::CommandManager m_commandManager;
    FileSystemManger* m_fileSystemManager;
    PreviewConnectManager* m_connectManager;
};


#endif /* PREVIEWTOOL_H */
