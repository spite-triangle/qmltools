#ifndef TESTFILEWATCHER_HPP
#define TESTFILEWATCHER_HPP

#include "common/doctest.hpp"

#include "utils/filesystemwatcher.h"

#include <QFileSystemWatcher>

using namespace Utils;

TEST_CASE("file watcher"){
    FileSystemWatcher* watcher = new FileSystemWatcher();
    watcher->addDirectory(FilePath::fromString("E:/workspace/qt servitor/tools/src/demo"), FileSystemWatcher::WatchAllChanges);
    watcher->addFile(FilePath::fromString("E:/workspace/qt servitor/tools/src/demo/main.qml"), FileSystemWatcher::WatchAllChanges);

    QObject::connect(watcher, &FileSystemWatcher::fileChanged, [](const QString &path){
        /* 
            1. 文件内容保存
            2. 命名
            3. 删除
         */
        qDebug() << "file change";
        qDebug() << path;
    });

    QObject::connect(watcher, &FileSystemWatcher::directoryChanged, [](const QString &path){
        /* 
            1. 文件夹内的文件内容修改
            2. 文件夹内增删文件
         */
        qDebug() << "directory change";
        qDebug() << path;
    });
}


#endif /* TESTFILEWATCHER_HPP */
