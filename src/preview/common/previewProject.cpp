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

    app.add_flag("-v,--verbose", m_setting.bLog, "Print more debug information to `log/run.log`.");
    app.add_flag("--quiet", m_setting.bConsoleLog, "Print console Message.")
        ->default_val(false);

    app.add_option("-z,--zoom", m_setting.fZoom, "Display zoom factor.")
        ->default_val(1.0)
        ->check(CLI::Range(0.2,10.0));

    app.add_option("-i,--interval",m_setting.uUpdateInterval,"Interval (ms) to update file change.")
        ->default_val(2000)
        ->check(CLI::Range(1000,10000));

    app.add_option_function<std::string>("-t,--target", 
        [&](const std::string & val){
            m_setting.strTargetFile = OwO::Utf8ToQString(val).trimmed();

            QFileInfo file(m_setting.strTargetFile);
            if(file.exists() == false || file.isExecutable() == false){
                throw CLI::ValidationError("target path isn't a valid executable file.");
            }
        },
        "File path. The target program will be launched for live preview.");

    app.add_option_function<std::string>("--cwd",
        [&](const std::string & val){
            m_setting.strTargetWorkFolder = OwO::Utf8ToQString(val).trimmed();
        }, 
        "The workspace folder of target program.");

    app.add_option_function<std::string>("-s,--socket", 
        [&](const std::string & val){
            m_setting.strSocketFile = OwO::Utf8ToQString(val).trimmed();
        }, 
        "The socket file of QML debug server.");

    app.add_option_function<std::string>("-h,--host",
        [&](const std::string & val){
            m_setting.strHost = OwO::Utf8ToQString(val).trimmed();
        },             
        "The host of QML debug server.")
        ->default_val("127.0.0.1");

    app.add_option("-p,--port", m_setting.uport,"The port of QML debug server.")
        ->default_val(2333)
        ->check(CLI::Range(0,65535));

    app.add_option_function<std::vector<std::string>>("--qrc",
        [&](const std::vector<std::string> & vals){
            for(auto & val :vals){
                QString strFile = OwO::Utf8ToQString(val).trimmed();
                if(QFileInfo(strFile).exists() == false) continue; 
                m_setting.setQrcFile.insert(std::move(strFile));
            }
        },
        "*.qrc file path. Multiple files are separated by `;`.")
        ->delimiter(';');

    app.add_option_function<std::vector<std::string>>("--search",
        [&](const std::vector<std::string> & vals){
            for(auto & val :vals){
                QString strFile = OwO::Utf8ToQString(val).trimmed();
                if(QDir(strFile).exists() == false) continue;
                m_setting.setQrcFile.insert(std::move(strFile));
            }
        },            
        "Extend asset search folder. Multiple folders are separated by `;`.")
        ->delimiter(';');

    app.add_option_function<std::string>("--limit",
        [&](const std::string & val){
            m_setting.strLimitedFolder = OwO::Utf8ToQString(val).trimmed();
            return true;
        }, 
        "Limit folder to search asset.");

    auto group = app.add_option_group("Required", "These options have to configure.");

    group->add_option_function<std::string>("--project",
        [&](const std::string & val){
            m_setting.strPojectFolder = OwO::Utf8ToQString(val).trimmed();
            QFileInfo dir(m_setting.strPojectFolder);
            if(dir.exists() == false || dir.isDir() == false){
                throw CLI::ValidationError("project folder isn't a valid folder path");
            }
        },
        "Project folder path.")
        ->required(true);

    group->add_option_function<std::string>("--focus",
        [&](const std::string & val){
            m_setting.strFocusLocalQml = OwO::Utf8ToQString(val).trimmed();

            QFileInfo file(m_setting.strFocusLocalQml);
            if(file.exists() == false && file.suffix() != "qml"){
                throw CLI::ValidationError("focus file isn't a valid qml file");
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
