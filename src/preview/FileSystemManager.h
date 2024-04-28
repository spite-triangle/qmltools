#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <QSet>
#include <QHash>
#include <QTimer>
#include <QObject>
#include <QString>
#include <QDateTime>

#include <functional>

#include "utils/qrcparser.h"
#include "utils/filesystemwatcher.h"
#include "qmljs/qmljsmodelmanagerinterface.h"

#include "common/previewDefines.h"
#include "debugClient/previewClient.h"

class FileSystemManger : public QObject{
    Q_OBJECT
public:
    using Ptr = QPointer<FileSystemManger>;
    using FileHandler = std::function<void (const Utils::FilePath &, int)>;
    using DirectoryHandler = std::function<void (const QStringList &, int)>;

    // 文件类型
    enum FILE_TYPE_E{
        FILE_TYPE_NONE, // 异常
        FILE_QML_JS,
        FILE_QRC,
        FILE_NORM // 一般文件
    };

public:
    FileSystemManger(QObject * parent = nullptr);

    bool init();

    // 将 strPath 路径的文件读取到内存
    QByteArray loadFile(const QString & strPath, bool & bRes);

    // 更新关注的 qml 路径
    void updateFocusQml();

    void addFile(const QString & strPath);

public slots:
    bool onPathRequested(const QString & strPath);

    // m_watcher 监控的文件发生了改变
    void onFileChanged(const QString & strPath);


signals:
    void sigRerun();
    void sigLoadUrl(const QUrl &url);
    void sigAnnounceFile(const QString &path, const QByteArray &contents);
    void sigAnnounceDirectory(const QString &path, const QStringList &entries);
    void sigAnnounceError(const QString &path);
    void sigClearCache();

private:

    /* 根据本地路径查找 qrc 路径 */
    QString findQrcUrlByParser(const QString & strSource);

    /* 根据 qrc 路径查找本地路径 */
    bool findSourceByParser(const QString & strQrc, FileHandler fileHandler, DirectoryHandler directoryHandler);

    FILE_TYPE_E inspectFileType(const QString & strSource);
    
    bool checkFileValid(const QString & strSource, const QByteArray & content, const FILE_TYPE_E & enType);

    void printErrorMessage(const QString & file, const QStringList & lstMsg);
    void printErrorMessage(const QString & file, const QList<QmlJS::DiagnosticMessage> & lstMsg);

private:
    QTimer* m_timer;
    QHash<QString, QDateTime> m_mapFileModifyTime;
    Utils::FileSystemWatcher m_watcher; // 文件监控
    FileFinderPtr_t m_pFinder; // 文件查找
    Utils::QrcCache m_qrcs; // qrc 管理器
};




#endif /* FILESYSTEMMANAGER_H */
