#ifndef LSPPROJECT_H
#define LSPPROJECT_H

#include <mutex>

#include <QUrl>
#include <QSet>
#include <QList>
#include <QObject>
#include <QLocale>
#include <QVersionNumber>

#include "utils/filepath.h"

#include "common/utils.h"
#include "common/QSingleton.hpp"


namespace ProjectExplorer 
{

class Project : public QObject, public QSingleton<Project>{
    Q_OBJECT
public:
    enum CHECK_TYPE_E : int
    {
        TYPE_EXIST = 1,
        TYPE_EXCUTABLE = 2,
        TYPE_FILE = 4,
        TYPE_FOLDER = 8
    };

    enum SDK_ASSET_E{
        SDK_FOLDER_QML = 1,
        SDK_TOOL_PLUGINDUMP = 2
    };

    struct PROJECT_SETTING_S
    {
        QString strName;  // 项目名
        QString strVersion; // 版本
        QString strPojectFolder; // 项目文件夹
        QString strTypeDescriptionFolder; // qml-type-descriptions
        QString strSdkFolder; // sdk 所在文件夹
        QStringList lstTargetFolder; // 目标文件所在路径
        QStringList lstQml2ImportPath; // QML2_IMPORT_PATH
        QStringList lstQrcPath; 
        QStringList lstSourceFolder;
        QStringList lstImportPaths;
        QVersionNumber versionSdk; // sdk 版本号
        uint uPort; // 服务端口
        bool bLog; // 是否打印本地日志
    };
    

public:
    Project(QObject * p = nullptr) : QObject(p){
        m_setting.strVersion = "qmllsp version 1.1.0";
    }

    MUT_FUNC_SET_GET(m_muteSetting, PROJECT_SETTING_S, m_setting, Settings);
    MUT_FUNC_GET(m_muteSetting, QString, m_setting.strName, Name);
    MUT_FUNC_GET(m_muteSetting, QString, m_setting.strVersion, Version);
    MUT_FUNC_GET(m_muteSetting, QString, m_setting.strPojectFolder, ProjectFolder);
    MUT_FUNC_SET_GET(m_muteSetting, QString, m_setting.strTypeDescriptionFolder, QmlTypeDescription);
    MUT_FUNC_SET_GET(m_muteSetting, QString, m_setting.strSdkFolder, SdkFolder);
    MUT_FUNC_SET_GET(m_muteSetting, QVersionNumber, m_setting.versionSdk, VersionSdk);
    MUT_FUNC_SET_GET(m_muteSetting, QStringList, m_setting.lstQml2ImportPath, Qml2ImportPath);
    MUT_FUNC_SET_GET(m_muteSetting, QStringList, m_setting.lstTargetFolder, TargetFolder);
    MUT_FUNC_SET_GET(m_muteSetting, QStringList, m_setting.lstQrcPath, QrcFiles);
    MUT_FUNC_SET_GET(m_muteSetting, QStringList, m_setting.lstSourceFolder, SourceFolder);
    MUT_FUNC_SET_GET(m_muteSetting, QStringList, m_setting.lstImportPaths, ImportPaths);
    MUT_FUNC_SET_GET(m_muteSetting, bool, m_setting.bLog, ExportLog);
    MUT_FUNC_SET_GET(m_muteSetting, uint, m_setting.uPort, Port);
   

    /* 解析命令行 */
    int parserCommand(int argc, char *argv[]);

    /* 从配置中查找父目录 */
    QString parentFolder(const QString & strPath); 

    /* sdk 下的资产目录 */
    QString sdkAssetPath(const SDK_ASSET_E & enType);

    void clearSourceFiles();
    void appendSourceFile(const Utils::FilePaths & paths);
    void appendSourceFile(const QString & path);
    bool containSourceFile(const QString & path);
    void removeSourceFile(const QString & path);
    size_t sizeSourceFile();
    QSet<Utils::FilePath> getSourceFiles();

private:

    bool checkPath(const QString & strPath, const std::string & name ,int nType, bool bThrow = true);
    QString formatPath(const std::string & str);

private:
    std::mutex m_muteSetting;
    PROJECT_SETTING_S m_setting;

    std::mutex m_muteSourceSet;
    QSet<Utils::FilePath> m_setSourceFiles; // 项目工程的所有源文件
};
   
} // namespace ProjectExplorer



#endif /* LSPPROJECT_H */
