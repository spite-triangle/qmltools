#include "common/previewProject.h"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "common/utils.h"
#include "common/argparse.hpp"
#include "previewProject.h"

void ProjectExplorer::Project::appendQrcFile(const QString &strPath)
{
    m_setting.setQrcFile.insert(strPath);
}
void ProjectExplorer::Project::appendExtendSearchFolder(const QString &strPath)
{
    m_setting.setExtendSearchFolder.insert(strPath);
}

bool ProjectExplorer::Project::parserCommand(int argc, char *argv[])
{
    m_setting.strRunFolder = QCoreApplication::applicationDirPath();

    auto args = util::argparser(R"(
The QML preview tool will listen project files dependent by the Quick application
and refreshe application live with these files change.
Files include *.qml, *.js, *.qrc, qmldir etc.
)");

    args.set_program_name("qmlpreviewtool")
        .add_help_option()
        .add_sc_option("", "--version", "show version info", [&]() {
            std::cout << "version " << OwO::QStringToUtf8(m_setting.strVersion) << std::endl;
        })
        .add_option("-v", "--verbose", "Print more debug information to `log/run.log`.")
        .add_option<std::string>("-l","--language","Configure language QLocale.","")
        .add_option<int64_t>("-i","--interval","Interval (ms) to update file change.", 2000)
        .add_option<std::string>("-t","--target", "File path. The target program to be started and previewed.","")
        .add_option<std::string>("","--cwd","The workspace folder of target program.","")
        .add_option<std::string>("-h","--host", "The host of QML debug server.","127.0.0.1")
        .add_option<int64_t>("-p","--port", "The port of QML debug server.",2333)
        .add_option<std::string>("-s","--socket", "The socket file of QML debug server.","")
        .add_option<std::string>("","--qrc",".qrc file path. Multiple files are separated by `;`.","")
        .add_option<std::string>("","--search","Extend asset search folder. Multiple folders are separated by `;`.","")
        .add_option<std::string>("","--limit","Limit folder to search asset.","")
        .add_named_argument<std::string>("project", "Project folder path.")
        .add_named_argument<std::string>("focus", "File path. The QML file be focused.")
        .parse(argc, argv);

    QDir root(m_setting.strRunFolder);

    if(args.has_option("--verbose")){
        logFolder();
        m_setting.bLog = true;
    }

    {
        int64_t nInterval = args.get_option_int64("--interval");
        if(nInterval <= 0){
            m_setting.uUpdateInterval = 2000;
        }
        m_setting.uUpdateInterval = nInterval;
    }

    if(args.get_option_string("--language").empty() == false){
        m_setting.language = QLocale(OwO::ToQString(args.get_option_string("--language")));
    }

    if(args.get_option_string("--target").empty() == false){
        m_setting.strTargetFile = root.absoluteFilePath(OwO::ToQString(args.get_option_string("--target")));
        QFileInfo file(m_setting.strTargetFile);
        if(file.exists() == false || file.isExecutable() == false) {
            CONSOLE_ERROR("the value of --target isn't a executable programe.");
            return false;
        }
    }
    
    if(args.get_option_string("--cwd").empty() == false){
        m_setting.strTargetWorkFolder = root.absoluteFilePath(OwO::ToQString(args.get_option_string("--cwd")));
        if(root.exists(m_setting.strTargetWorkFolder) == false){
            CONSOLE_ERROR("the value of --cwd isn't a valid folder.");
            return false;
        }
    }
    
    m_setting.strSocketFile = OwO::ToQString(args.get_option_string("--socket"));
    if(m_setting.strSocketFile.isEmpty()){
        m_setting.uport = args.get_option_int64("--port");
        m_setting.strHost = OwO::ToQString(args.get_option_string("--host"));
    }
    
    if(args.get_option_string("--search").empty() == false){
        QStringList lstFolder = OwO::ToQString(args.get_option_string("--search")).split(";");
        for (auto & folder: lstFolder)
        {
            if(root.exists(folder) == false)  continue;

            m_setting.setExtendSearchFolder.insert(root.absoluteFilePath(folder));
        }
    }
    
    if(args.get_option_string("--qrc").empty() == false){
        QStringList lstFiles = OwO::ToQString(args.get_option_string("--qrc")).split(";");
        for(auto & file : lstFiles){
            QFileInfo info(root.absoluteFilePath(file));
            if(info.exists() == false || info.suffix() != "qrc") continue;

            m_setting.setQrcFile.insert(info.absoluteFilePath());
        } 
    }

    if(args.get_option_string("--limit").empty() == false){
        QString str = root.absoluteFilePath(OwO::ToQString(args.get_option_string("--limit")));
        if(root.exists(root.absoluteFilePath(str)) == true){
            m_setting.strLimitedFolder = root.absoluteFilePath(str);
        }
    }
    
    {
        QFileInfo file(root.absoluteFilePath(OwO::ToQString(args.get_argument_string("focus"))));
        if(file.exists() == false || file.suffix() != "qml"){
            CONSOLE_ERROR("the value of focus isn't a QML file.");
            return false;
        }

        m_setting.strFocusLocalQml = file.absoluteFilePath();
    }
    {
        QString strFolder = OwO::ToQString(args.get_argument_string("project"));
        if(root.exists(strFolder) == false){
            CONSOLE_ERROR("the value of project isn't a project folder.");
            return false;
        }
        m_setting.strPojectFolder = root.absoluteFilePath(strFolder);
    }
    return true;
}

bool ProjectExplorer::Project::logFolder()
{
    QDir root(QCoreApplication::applicationDirPath());
    root.mkdir("log");
    return true;
}
