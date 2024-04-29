#ifndef PREVIEWPROJECT_H
#define PREVIEWPROJECT_H

#include <QSet>
#include <QList>
#include <QObject>
#include <QLocale>

#include "common/utils.h"
#include "common/singleton.hpp"

namespace ProjectExplorer 
{

class Project : public QObject, public Singleton<Project>{
    Q_OBJECT
public:
    struct PROJECT_SETTING_S
    {
        QString strName;  // 项目名
        QString strVersion; // 版本
        QString strFocusLocalQml; // 在线预览的入口 qml 文件本地路径
        QString strFocusQrcQml; // 在线预览的入口 qml 文件 qrc 路径，qml engine 能识别的路径
        QString strPojectFolder; // 项目文件夹
        QString strLimitedFolder; // 只在指定的文件夹下查找需要的资源
        QString strTargetFile;
        QString strTargetWorkFolder; // 目标程序工作目录
        QString strRunFolder; // preview 所在目录
        QString strSocketFile;
        QString strHost;
        size_t  uport;
        size_t  uUpdateInterval;
        QSet<QString> setQrcFile; // qrc 文件路径
        QSet<QString> setExtendSearchFolder; // 其他额外存在资源的文件夹
        QLocale language; 
        float fZoom;
        float nFpsIntreval = 1.0;
        bool bErrorReload = true;
        bool bConsoleLog = true; // 控制台打印
        bool bLog = false;
        bool bShowFps = false;
    };
    

public:
    Project(QObject * p = nullptr) : QObject(p){
        m_setting.strVersion = "qmlpreview version 1.0.0";
    }

    FUNC_GET(QString, m_setting.strName, Name);
    FUNC_GET(QString, m_setting.strVersion, Version);
    FUNC_SET_GET(QLocale, m_setting.language, Language);
    FUNC_SET_GET(QString, m_setting.strFocusLocalQml, FocusLocalQml);
    FUNC_SET_GET(QString, m_setting.strFocusQrcQml, FocusQrcQml);
    FUNC_SET_GET(QString, m_setting.strPojectFolder, ProjectFolder);
    FUNC_SET_GET(QString, m_setting.strLimitedFolder, LimitedFolder);
    FUNC_SET_GET(QString, m_setting.strTargetWorkFolder, TargetWorkFolder);
    FUNC_SET_GET(QString, m_setting.strRunFolder, RunFolder);
    FUNC_SET_GET(QString, m_setting.strTargetFile, Target);
    FUNC_SET_GET(QString, m_setting.strSocketFile, SocketFile);
    FUNC_SET_GET(QString, m_setting.strHost, Host);
    FUNC_SET_GET(size_t, m_setting.uport, Port);
    FUNC_SET_GET(float, m_setting.fZoom, Zoom);
    FUNC_SET_GET(float, m_setting.nFpsIntreval, FpsInterval);
    FUNC_SET_GET(bool, m_setting.bErrorReload, ErrorReload);
    FUNC_SET_GET(bool, m_setting.bConsoleLog, ConsoleLog);
    FUNC_SET_GET(bool, m_setting.bShowFps, ShowFPS);
    FUNC_SET_GET(size_t, m_setting.uUpdateInterval, UpdateInterval);
    FUNC_SET_GET(bool, m_setting.bLog, ExportLog);
    FUNC_GET(QSet<QString>, m_setting.setQrcFile, QrcFiles);
    FUNC_GET(QSet<QString>, m_setting.setExtendSearchFolder, ExtendSearchFolder);

    void appendQrcFile(const QString & strPath);
    void appendExtendSearchFolder(const QString & strPath);

    /* 解析命令行 */
    int parserCommand(int argc, char *argv[]);

private:
    PROJECT_SETTING_S m_setting;
};
   
} // namespace ProjectExplorer



#endif /* PREVIEWPROJECT_H */
