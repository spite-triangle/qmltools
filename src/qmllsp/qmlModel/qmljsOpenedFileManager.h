#ifndef QMLJSOPENEDFILEMANAGER_H
#define QMLJSOPENEDFILEMANAGER_H

#include <QHash>
#include <QPair>
#include <QString>


#include "utils/filepath.h"
#include "common/singleton.hpp"
#include "common/utils.h"

namespace QmlJS
{

class OpenedFileManager : public Singleton<OpenedFileManager>{
public:
    using Table_t = QHash<Utils::FilePath, QPair<QString, int>>;
public:

    MUT_FUNC_GET(m_muteOpenedFile, Table_t, m_openedFiles, OpenedFiles);

    void openFile(const QString & path, int revision = 0);
    void closeFile(const QString & path);
    void updateFile(const QString & path, const QString & content);
    QString fileContent(const QString & path);

private:
    std::mutex m_muteOpenedFile;
    Table_t m_openedFiles;
};

} // namespace QmlJS


#endif // QMLJSOPENEDFILEMANAGER_H
