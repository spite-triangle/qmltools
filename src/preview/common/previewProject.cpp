#include "common/previewProject.h"

#include <QDir>
#include <QFileInfo>

#include "common/utils.h"
#include "common/CLI11.hpp"
#include "common/previewLog.hpp"
#include "previewProject.h"

void ProjectExplorer::Project::appendQrcFile(const QString &strPath)
{
    m_setting.setQrcFile.insert(strPath);
}
void ProjectExplorer::Project::appendExtendSearchFolder(const QString &strPath)
{
    m_setting.setExtendSearchFolder.insert(strPath);
}

int ProjectExplorer::Project::parserCommand(int argc, char *argv[])
{

    CLI::App app(R"(
The QML preview tool will listen project files dependent by the Quick application
and refreshe application live with these files change.
Files include *.qml, *.js, *.qrc, qmldir etc.
)", "qmlpreviewtool");

    argv = app.ensure_utf8(argv);
    app.set_help_flag("-?,--help", "Print this help message and exit.");
    app.set_version_flag("--version",OwO::ToStdString(m_setting.strVersion));

    app.add_flag("-v,--verbose", m_setting.bLog, "Print more debug information to `log/preview.log`.");
    app.add_flag("--quiet", m_setting.bConsoleLog, "Print console Message.");
    app.add_flag("--ignore", m_setting.bErrorReload, "Turn off refresh preview interface when error or empty file is reloaded successfully.\nIf file loading falls into death loop, you should need this.");

    app.add_option("-z,--zoom", m_setting.fZoom, "Display zoom factor.")
        ->default_val(1.0)
        ->check(CLI::Range(0.2,10.0));

    app.add_option("-i,--interval",m_setting.uUpdateInterval,"Interval (ms) to update file change.")
        ->default_val(1000)
        ->check(CLI::Range(1000,10000));

    app.add_option_function<std::string>("-t,--target", 
        [&](const std::string & val){
            m_setting.strTargetFile = formatPath(val);

            checkPath(m_setting.strTargetFile, "target",CHECK_TYPE_E::TYPE_EXCUTABLE | CHECK_TYPE_E::TYPE_EXIST);
        },
        "File path. The target program will be launched for live preview.");

    app.add_option_function<std::string>("--cwd",
        [&](const std::string & val){
            m_setting.strTargetWorkFolder = formatPath(val);
            checkPath(m_setting.strTargetWorkFolder,"cwd" ,CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
        }, 
        "The workspace folder of target program.");

    app.add_option_function<std::string>("-s,--socket", 
        [&](const std::string & val){
            m_setting.strSocketFile = formatPath(val);
        }, 
        "The socket file of QML debug server.");

    app.add_option_function<std::string>("-h,--host",
        [&](const std::string & val){
            m_setting.strHost = OwO::ToQString(val);
        },             
        "The host of QML debug server.")
        ->default_val("127.0.0.1");

    app.add_option("-p,--port", m_setting.uport,"The port of QML debug server.")
        ->default_val(2333)
        ->check(CLI::Range(0,65535));

    app.add_option_function<std::vector<std::string>>("--qrc",
        [&](const std::vector<std::string> & vals){
            for(auto & val :vals){
                QString strFile = formatPath(val);
                checkPath(strFile,"qrc", CHECK_TYPE_E::TYPE_FILE | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.setQrcFile.insert(std::move(strFile));
            }
        },
        "*.qrc file path. Multiple files are separated by `,`.")
        ->delimiter(',');

    app.add_option_function<std::vector<std::string>>("--search",
        [&](const std::vector<std::string> & vals){
            for(auto & val :vals){
                QString strFile = formatPath(val);
                checkPath(strFile, "search",CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.setQrcFile.insert(std::move(strFile));
            }
        },            
        "Extend asset search folder. Multiple folders are separated by `,`.")
        ->delimiter(',');

    auto group = app.add_option_group("Required", "These options have to configure.");

    group->add_option_function<std::string>("--project",
        [&](const std::string & val){
            m_setting.strPojectFolder = formatPath(val);
            QFileInfo dir(m_setting.strPojectFolder);
            checkPath(m_setting.strPojectFolder,"project" ,CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
        },
        "Project folder path.")
        ->required(true);

    group->add_option_function<std::string>("--focus",
        [&](const std::string & val){
            m_setting.strFocusLocalQml = formatPath(val);

            QFileInfo file(m_setting.strFocusLocalQml);
            if(file.exists() == false && file.suffix() != "qml"){
                throw CLI::ValidationError("focus: focus file isn't a valid qml file.");
            }
        },
        "File path. The primary QML file will be previewed.")
        ->required(true);

    app.get_option("--host")
        ->excludes(app.get_option("--socket"))
        ->needs(app.get_option("--port"));

    app.get_option("--port")
        ->excludes(app.get_option("--socket"))
        ->needs(app.get_option("--host"));

    std::shared_ptr<CLI::FormatterBase> fmt = std::make_shared<CLI::Formatter>();
    fmt->column_width(50);
    app.formatter(fmt);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }  

    return 0;
}

QString ProjectExplorer::Project::parentFolder(const QString &strPath)
{
    auto src = QFileInfo(strPath.trimmed()).canonicalFilePath();

    auto handler = [&](const QString & folder){
        if(folder.isEmpty() == false && src.startsWith(folder, Qt::CaseInsensitive)){
            return true;
        }
        return false;
    };

    // 项目文件夹
    if(handler(m_setting.strPojectFolder)){
        return m_setting.strPojectFolder;
    }

    // 工作目录
    if(handler(m_setting.strTargetWorkFolder)){
        return m_setting.strTargetWorkFolder;
    }

    // 其他文件夹
    for (auto & strFolder : m_setting.setExtendSearchFolder)
    {
        if(handler(strFolder)){
            return strFolder;
        }
    }

    return QString();
}

void ProjectExplorer::Project::checkPath(const QString &strPath, const std::string & name,int nType)
{
    QFileInfo file(strPath);
    std::string error;

    BLOCK{
        if(nType & CHECK_TYPE_E::TYPE_EXIST && file.exists( ) == false){
            error = "Path does not exist.";
            break;
        }
        if(nType & CHECK_TYPE_E::TYPE_EXCUTABLE && file.isExecutable() == false){
            error = "File isn't a valid executable program.";
            break;
        }
        if(nType & CHECK_TYPE_E::TYPE_FILE && file.isFile() == false){
            error = "Path isn't a valid file path.";
            break;
        }
        if(nType & CHECK_TYPE_E::TYPE_FOLDER && file.isDir() == false){
            error = "Path isn't a valid folder path.";
            break;
        }
        return;
    }

    throw CLI::ValidationError(name ,error + " Check your settings and folder separator is `\\` or `/`.");
}

QString ProjectExplorer::Project::formatPath(const std::string &str)
{
    return QFileInfo(OwO::Utf8ToQString(str).trimmed()).absoluteFilePath();
}
