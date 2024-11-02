
#include "qmljsOpenedFileManager.h"

#include <QByteArray>

#include "utils/filepath.h"
#include "common/lspLog.h"


namespace Private
{
    QByteArray loadFile(const QString &strPath, bool &bRes)
    {
        bRes = true;
        QByteArray content;
        QFile file(strPath);

        ASSERT_RETURN(file.exists() == true, "file is not exist", bRes = false, content);

        // NOTE - Qfile 读取文件有概率会为空
        BLOCK_TRY(4,0){
            ASSERT_RETURN(file.open(QIODevice::ReadOnly) == true, "failed to open file", bRes = false, content);
            content = file.readAll();
            if(content.isEmpty() == false) TRY_BREAK;
            file.close();
        }
        return content;
    }
 
} // namespace Private


namespace QmlJS {

void OpenedFileManager::openFile(const QString & path, int revision) {
    auto filePath = Utils::FilePath::fromString(path);

    bool bFlag = false;
    auto content = Private::loadFile(path, bFlag);
    if(bFlag == false) return;

    std::lock_guard<std::mutex> lock(m_muteOpenedFile);
    if(m_openedFiles.contains(filePath) == true) return;
    m_openedFiles.insert(filePath, {content, revision});
}

void OpenedFileManager::closeFile(const QString & path) {
    auto filePath = Utils::FilePath::fromString(path);

    std::lock_guard<std::mutex> lock(m_muteOpenedFile);
    if(m_openedFiles.contains(filePath)){
        m_openedFiles.remove(filePath);
    } 
}

void OpenedFileManager::updateFile(const QString & path, const QString & content, int revision) {
    auto filePath = Utils::FilePath::fromString(path);

    std::lock_guard<std::mutex> lock(m_muteOpenedFile);
    if(m_openedFiles.contains(filePath)){
        m_openedFiles[filePath].first = content;
        m_openedFiles[filePath].second = revision;
    } 
}

QString OpenedFileManager::fileContent(const QString &path)
{
    auto filePath = Utils::FilePath::fromString(path);

    std::lock_guard<std::mutex> lock(m_muteOpenedFile);
    if(m_openedFiles.contains(filePath)){
        return m_openedFiles[filePath].first;
    }
    return QString();
}

}