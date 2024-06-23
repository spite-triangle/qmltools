
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

    app.add_option<uint>("-p,--port", m_setting.uPort, "server port.");

    app.add_option_function<std::string>("--typeDescription", 
        [&](const std::string & val){
            auto path = formatPath(val);
            checkPath(path, "typeDescription", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
            m_setting.strTypeDescriptionFolder = path; 
        }, 
        "The `qml-type-descriptions` folder.");

    app.add_option_function<std::string>("--sdk",
        [&](const std::string & val){
            auto path = formatPath(val);
            checkPath(path, "sdk", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
            m_setting.strSdkFolder = path;
        },
        "The folder of Qt Sdk.");

    app.add_option_function<std::vector<std::string>>("--targetFolder",
        [&](const std::vector<std::string> & vals){
            for(auto & val : vals){
                auto path = formatPath(val);
                checkPath(path, "targetFolder", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.lstTargetFolder.append(path);
            }
        },
        "The folders including target file. Multiple folders are separated by `,`.")
        ->delimiter(','); 
    
    app.add_option_function<std::vector<std::string>>("--qml2imports",
        [&](const std::vector<std::string> & vals){
            for (auto & val : vals)
            {
                auto path = formatPath(val);
                checkPath(path, "qml2import", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.lstQml2ImportPath.append(path);
            }
            
        },
        "The `QML2_IMPORT_PATH` folders. Multiple folders are separated by `,`.")
        ->delimiter(','); 

    app.add_option_function<std::vector<std::string>>("--import",
        [&](const std::vector<std::string> & vals){
            for (auto & val : vals)
            {
                auto path = formatPath(val);
                checkPath(path, "imports", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.lstImportPaths.append(path);
            }
            
        },
        "The qml import folders. Multiple folders are separated by `,`.")
        ->delimiter(','); 
    
    app.add_option_function<std::vector<std::string>>("--qrc",
        [&](const std::vector<std::string> & vals){
            for (auto & val : vals)
            {
                auto path = formatPath(val);
                checkPath(path, "qrc", CHECK_TYPE_E::TYPE_FILE | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.lstQrcPath.append(path);
            }
            
        },
        "The `*.qrc` resource files. Multiple folders are separated by `,`.")
        ->delimiter(','); 

         
    app.add_option_function<std::vector<std::string>>("--src",
        [&](const std::vector<std::string> & vals){
            for (auto & val : vals)
            {
                auto path = formatPath(val);
                checkPath(path, "src", CHECK_TYPE_E::TYPE_FOLDER | CHECK_TYPE_E::TYPE_EXIST);
                m_setting.lstSourceFolder.append(path);
            }
            
        },
        "The folders including qml source project (e.g. `*.qml`, `.js`,`qmldir`). Multiple folders are separated by `,`.")
        ->delimiter(','); 

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


QString Project::sdkAssetPath(const Project::SDK_ASSET_E &enType)
{
    std::lock_guard<decltype(m_muteSetting)> lock(m_muteSetting);

    QString strFile;
    switch (enType)
    {
    case SDK_ASSET_E::SDK_FOLDER_QML:  strFile = "qml";  break;
    case SDK_ASSET_E::SDK_TOOL_PLUGINDUMP:  strFile = "bin/qmlplugindump.exe";  break;
    default:
        break;
    }

    return m_setting.strSdkFolder + "/" + strFile;
}


void Project::clearSourceFiles() {
    std::lock_guard lock(m_muteSourceSet);
    m_setSourceFiles.clear();
}

void Project::appendSourceFile(const Utils::FilePaths & paths) {
    std::lock_guard lock(m_muteSourceSet);
    for (auto & path : paths)
    {
        m_setSourceFiles.insert(path);
    }
    
}

void Project::appendSourceFile(const QString & path) {
    std::lock_guard lock(m_muteSourceSet);
    m_setSourceFiles.insert(Utils::FilePath::fromString(path));
}

bool Project::containSourceFile(const QString & path) {
    std::lock_guard lock(m_muteSourceSet);
    return m_setSourceFiles.contains(Utils::FilePath::fromString(path));
}

void Project::removeSourceFile(const QString & path) {
    std::lock_guard lock(m_muteSourceSet);
    m_setSourceFiles.remove(Utils::FilePath::fromString(path));
}

size_t Project::sizeSourceFile()
{
    std::lock_guard lock(m_muteSourceSet);
    return m_setSourceFiles.size();
}

QSet<Utils::FilePath> Project::getSourceFiles()
{
    std::lock_guard lock(m_muteSourceSet);
    return m_setSourceFiles;
}

}