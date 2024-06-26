# Qt Creator

# preview

> - [工程项目](https://github.com/qt-creator/qt-creator/tree/master/src/plugins/qmlpreview)

## 基本流程

1. QmlPreviewConnectionManager::connectionOpened
2. zoomFactor
3. QmlPreviewPluginPrivate::checkFile 解析当前选中的 qml
4. QmlPreviewParser::parse 检查语法 QmlJS::Document::MutablePtr qmljsDoc
5. QmlPreviewPluginPrivate::triggerPreview 语法检测成功触发预览
6. QmlPreviewPlugin::updatePreviews --> QmlPreviewConnectionManager::createPreviewClient 下的 QmlPreviewConnectionManager::loadFile
    - qtquickcontrols2.conf 排除该文件
    - 将路径替换成 qrc: 路径
7. QmlPreviewClient::announceFile 发送文件 qml，（可以不用发送）

8. 之后界面刷新：
   1. 检测 qml 语法是否正确
   2. announceFile 更新
   3. load 通知引擎重新加载


## 文件内容改变

1. 通知文件修改

```cpp
Core::IDocument::contentsChanged
```

2. 筛选文件类型

```cpp
    QmlJS::Dialect::Enum dialect = QmlJS::Dialect::AnyLanguage;
    Core::IDocument *doc = m_lastEditor->document();
    const QString mimeType = doc->mimeType();
    if (mimeType == QmlJSTools::Constants::JS_MIMETYPE)
        dialect = QmlJS::Dialect::JavaScript;
    else if (mimeType == QmlJSTools::Constants::JSON_MIMETYPE)
        dialect = QmlJS::Dialect::Json;
    else if (mimeType == QmlJSTools::Constants::QML_MIMETYPE)
        dialect = QmlJS::Dialect::Qml;
//  --- Can we detect those via mime types?
//  else if (mimeType == ???)
//      dialect = QmlJS::Dialect::QmlQtQuick1;
//  else if (mimeType == ???)
//      dialect = QmlJS::Dialect::QmlQtQuick2;
    else if (mimeType == QmlJSTools::Constants::QBS_MIMETYPE)
        dialect = QmlJS::Dialect::QmlQbs;
    else if (mimeType == QmlJSTools::Constants::QMLPROJECT_MIMETYPE)
        dialect = QmlJS::Dialect::QmlProject;
    else if (mimeType == QmlJSTools::Constants::QMLTYPES_MIMETYPE)
        dialect = QmlJS::Dialect::QmlTypeInfo;
    else if (mimeType == QmlJSTools::Constants::QMLUI_MIMETYPE)
        dialect = QmlJS::Dialect::QmlQtQuick2Ui;
    else
        dialect = QmlJS::Dialect::NoLanguage;

    // We cannot dynamically load changes in qtquickcontrols2.conf
    filename.endsWith("qtquickcontrols2.conf");
```

3. 非 qml 类文件与 js 文件不检测语法

```cpp
    QmlJS::Dialect(dialect).isQmlLikeOrJsLanguage()
```

4.  是 qml 类文件与 js 文件语法检测，失败则不更新

```cpp
    QmlJS::Document::MutablePtr qmljsDoc = QmlJS::Document::create(Utils::FilePath::fromString(name),
                                                                   dialect);
    qmljsDoc->setSource(QString::fromUtf8(contents));
    if (qmljsDoc->parse())
```

5. 检测通过就更新

```cpp
        bool success = false;

        // 被修改的 changedFile 本地原始资源路径映射 qrc 路径，不存在 qrc 返回也是本地路径
        const QString remoteChangedFile = m_targetFileFinder.findPath(changedFile, &success);
        if (success)
            m_qmlPreviewClient->announceFile(remoteChangedFile, contents);
        else
            m_qmlPreviewClient->clearCache();

        // 当前预览文件重新加载
        m_lastLoadedUrl = m_targetFileFinder.findUrl(filename);
        m_qmlPreviewClient->loadUrl(m_lastLoadedUrl);
```

## 文件修改


```cpp
// 直接使用就行
 Utils::FileSystemWatcher m_fileSystemWatcher;

    // 文件名修改、文件增删靠 m_fileSystemWatcher 进行通知
    connect(&m_fileSystemWatcher, &Utils::FileSystemWatcher::fileChanged,
                     m_qmlPreviewClient.data(), [this](const QString &changedFile) {
        if (!m_fileLoader || !m_lastLoadedUrl.isValid())
            return;

        bool success = false;

        const QByteArray contents = m_fileLoader(changedFile, &success);
        if (!success)
            return;

        if (!m_fileClassifier(changedFile)) {
            emit restart();
            return;
        }

        const QString remoteChangedFile = m_targetFileFinder.findPath(changedFile, &success);
        if (success)
            m_qmlPreviewClient->announceFile(remoteChangedFile, contents);
        else
            m_qmlPreviewClient->clearCache();

        m_qmlPreviewClient->loadUrl(m_lastLoadedUrl);
    });
```

```cpp
    // qml 发现未知文件时，会通知客户端查找，并把查到的文件添加到  m_fileSystemWatcher 中进行监控
  connect(m_qmlPreviewClient.data(),
            &QmlPreviewClient::pathRequested,
            this,
            [this](const QString &path) {
                const bool found = m_projectFileFinder.findFileOrDirectory(
                    Utils::FilePath::fromString(path),
                    [&](const Utils::FilePath &filename, int confidence) {
                        if (m_fileLoader && confidence == path.length()) {
                            bool success = false;
                            QByteArray contents = m_fileLoader(filename.toFSPathString(), &success);
                            if (success) {
                                if (!m_fileSystemWatcher.watchesFile(filename)) {
                                    m_fileSystemWatcher
                                        .addFile(filename,
                                                 Utils::FileSystemWatcher::WatchModifiedDate);
                                }
                                m_qmlPreviewClient->announceFile(path, contents);
                            } else {
                                m_qmlPreviewClient->announceError(path);
                            }
                        } else {
                            m_qmlPreviewClient->announceError(path);
                        }
                    },
                    [&](const QStringList &entries, int confidence) {
                        if (confidence == path.length())
                            m_qmlPreviewClient->announceDirectory(path, entries);
                        else
                            m_qmlPreviewClient->announceError(path);
                    });

                if (!found)
                    m_qmlPreviewClient->announceError(path);
            });

```


```cpp
// qrc 文件夹映射，需要提前自己解析
FileInProjectFinder::addMappedPath

// 项目根目录
FileInProjectFinder::setProjectDirectory

// 项目的 qml 路径，不用指定
FileInProjectFinder::setAdditionalSearchDirectories
```


## Locale 流程


```cpp

// 1. 设置 locale
QmlPreviewPlugin::setLocaleIsoCode

// 2. 触发信号 
QmlPreviewPlugin::localeIsoCodeChanged
QmlPreviewRunner::language
QmlPreviewConnectionManager::language

// 3. 客户端更新
m_lastUsedLanguage = locale;
// findValidI18nDirectoryAsUrl does not work if we didn't load any file
// service expects a context URL.
if (!m_lastLoadedUrl.isEmpty()) {
    // Search the parent directories of the last loaded URL for i18n files.
    m_qmlDebugTranslationClient->changeLanguage(findValidI18nDirectoryAsUrl(locale), locale);
}
```


# debugger

> [qml debugger](https://github.com/qt-creator/qt-creator/tree/master/src/plugins/debugger/qml)




QmlDebug::QmlDebugConnectionManager 创建连接，给  QmlDebug::QmlDebugClient 使用

qmlengine.cpp 下的 QmlEnginePrivate : public QmlDebugClient 就是调试器实现

# QmlJS

## ModelManagerInterface

### 项目刷新

```cpp

/* 系统配置修改时，用于参数刷新 */
ModelManagerInterface::ProjectInfo ModelManager::defaultProjectInfoForProject(
    Project *project, const FilePaths &hiddenRccFolders) const
{
    ModelManagerInterface::ProjectInfo projectInfo;
    projectInfo.project = project;    // 自定义 project
    projectInfo.qmlDumpEnvironment = Utils::Environment::systemEnvironment();
    /* 
        Utils::mimeTypeForFile(filePath,MimeMatchMode::MatchExtension).name() 类型为以下结果
        Constants::QML_MIMETYPE ,
        Constants::QBS_MIMETYPE,
        Constants::QMLPROJECT_MIMETYPE,
        Constants::QMLTYPES_MIMETYPE,
        Constants::QMLUI_MIMETYPE
     */
    projectInfo.sourceFiles;

    // 有 qmlplugindump.exe 就启用
    projectInfo.tryQmlDump
    projectInfo.qmlDumpPath
    projectInfo.qmlDumpEnvironment.appendOrSet("QML2_IMPORT_PATH", environment("QML2_IMPORT_PATH"), ":");

    //   ((qtVersion() > QVersionNumber(4, 8, 4) && qtVersion() < QVersionNumber(5, 0, 0)) || qtVersion() >= QVersionNumber(5, 1, 0));
    // 满足上述条件，就为 true
    projectInfo.qmlDumpHasRelocatableFlag

    // 版本号 5.15.2
    projectInfo.qtVersionString

    // 应用程序所在的绝对路径，需要在项目文件夹下
    projectInfo.applicationDirectories;

    // qml sdk 路径
    projectInfo.qtQmlPath;

    projectInfo.generatedQrcFiles // 根据 rcc 文件对应的 qrc 文件
    projectInfo.allResourceFiles  // qrc 文件路径， Utils::filteredUnique 进行去重
    projectInfo.activeResourceFiles 
    projectInfo.resourceFileContents // qrc 路径 -> qrc 内容

    // 
    projectInfo.importPaths.maybeInsert(FilePath::fromString(path),
                                                QmlJS::Dialect::Qml);


    // 根据 qmljsbundleprovider.h 进行创建
    projectInfo.extendedBundle
    projectInfo.activeBundle;
}


/* 
    1. qml 文件名修改
    2. 新 qml 文件添加
    3. qml 文件删除
    4. 配置修改
 */
void QmakeBuildSystem::updateQmlJSCodeModel(){

    // 配置修改刷新设置
    QmlJS::ModelManagerInterface::ProjectInfo projectInfo
        = modelManager->defaultProjectInfoForProject(project(),
                                                     project()->files(Project::HiddenRccFolders));

    // 根据 projectInfo 配置，重新设置项目
    // 会更新删除的文件和新添加的文件
    ModelManagerInterface::updateProjectInfo()

    // projectInfo 更新后的信号
    // 刷新所有源文件
    ModelManagerInterface::projectInfoUpdated
    ModelManagerInterface::updateSourceFiles
}

```

### 文件更新

```cpp
    // 文档内容被改变，已经保存
    QTextDocument::contentsChanged

    // qrc 需要实现异常检测

    // qml, qrc 文件修改，就调用。会触发 ModelManagerInterface::documentUpdated
    ModelManagerInterface::updateSourceFiles

    // 报告文件处理结果信号
    ModelManagerInterface::documentUpdated

    // 更新当前正操作文件的 SemanticInfo 用于语法提示、查找定义、重命名等操作
    SemanticInfoUpdater::update
    SemanticInfoUpdater::makeNewSemanticInfo
```


```cpp
    // 文档存在被修改的状态，可能还未保存
    QTextDocument::modificationChanged

    // model 更新
    ModelManagerInterface::fileChangedOnDisk
```