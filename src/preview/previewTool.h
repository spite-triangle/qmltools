#ifndef PREVIEWTOOL_H
#define PREVIEWTOOL_H

#include <QObject>

#include "FileSystemManager.h"
#include "debugClient/previewConnectManager.h"

class PreviewTool : public QObject{
    Q_OBJECT
public:
    PreviewTool(QObject * parent = nullptr);

    bool init();


    void connectServer(const QString & strServer);


public slots:
    void onDebugServiceUnavailable(const QString & name);
    void onApplicationQuit();

private:
    bool m_bQuit;
    FileSystemManger* m_fileSystemManager;
    PreviewConnectManager* m_connectManager;
};


#endif /* PREVIEWTOOL_H */
