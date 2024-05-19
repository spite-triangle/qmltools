
#include "lspProject.h"

#include <QFileInfo>

#include "common/CLI11.hpp"

namespace ProjectExplorer {

int Project::parserCommand(int argc, char *argv[]) {

    CLI::App app(R"(
The tool is a QML Language Server.
)", "qmllsp");

    argv = app.ensure_utf8(argv);
    app.set_help_flag("-?,--help", "Print this help message and exit.");
    app.set_version_flag("--version",OwO::ToStdString(m_setting.strVersion));

    app.add_flag("-v,--verbose", m_setting.bLog, "Print more debug information to `log/qmllsp.log`.");

    app.add_option_function<std::string>("-s,--socket", 
        [&](const std::string & val){
            m_setting.strSocketFile = OwO::ToQString(val).trimmed();
        }, 
        "The socket file of language server.");

    app.add_option<uint>("-p,--port", m_setting.uPort, "server port.");

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





}