#ifndef LSPPROJECT_H
#define LSPPROJECT_H

#include <QUrl>
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
    enum CHECK_TYPE_E : int
    {
        TYPE_EXIST = 1,
        TYPE_EXCUTABLE = 2,
        TYPE_FILE = 4,
        TYPE_FOLDER = 8
    };

    struct PROJECT_SETTING_S
    {
        QString strName;  // 项目名
        QString strVersion; // 版本
        QString strPojectFolder; // 项目文件夹
        QString strSocketFile; // 通信 socket 文件
        uint uPort; // 服务端口
        bool bLog; // 是否打印本地日志
    };
    

public:
    Project(QObject * p = nullptr) : QObject(p){
        m_setting.strVersion = "qmllsp version 1.0.0";
    }

    FUNC_GET(QString, m_setting.strName, Name);
    FUNC_GET(QString, m_setting.strVersion, Version);
    FUNC_SET_GET(QString, m_setting.strSocketFile, SocketFile);
    FUNC_SET_GET(bool, m_setting.bLog, ExportLog);
    FUNC_SET_GET(uint, m_setting.uPort, Port);
   

    /* 解析命令行 */
    int parserCommand(int argc, char *argv[]);

    /* 从配置中查找父目录 */
    QString parentFolder(const QString & strPath); 
private:

    void checkPath(const QString & strPath, const std::string & name ,int nType);
    QString formatPath(const std::string & str);

private:
    PROJECT_SETTING_S m_setting;
};
   
} // namespace ProjectExplorer



#endif // LSPPROJECT_H
