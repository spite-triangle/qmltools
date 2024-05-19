# API

# QrcParser

## 接口

```cpp

// 根据 qrc 的文件获取解析器
// path : qrc 路径
// contents：qrc 文件内容，可以不指定，让 parser 自己读取 qrc 内容
static QrcParser::Ptr QrcParser::parseQrcFile(const QString &path, const QString &contents);

// path: qrc 文件夹
// res: map<qrc 文件名 , 文件真实路径>
// addDirs: res 返回结果是否有 qrc 文件夹
// QLocale: 筛选器,  <qresource lang="fr"> ，不指定则解析全部
void QrcParser::collectFilesInPath(const QString &path, QMap<QString,QStringList> *res, bool addDirs,const QLocale *locale) const;

// 根据 qrcPath 查找源文件路径
void QrcParser::collectFilesAtPath(const QString &path, QStringList *res, const QLocale *locale) const;
// 根据源文路径查找 qrcpath
 void collectResourceFilesForSourceFile(const QString &sourceFile, QStringList *results,
                                           const QLocale *locale = nullptr) const;

// 根据 qrcPath 查找第一个满足条件的路径
QString QrcParser::firstFileAtPath(const QString &path, const QLocale &locale) const;

// qrc 错误信息检测
QStringList errorMessages() const;

// qrc 里面设置哪些 `lang=""`
QStringList languages() const;

// 有效性
bool isValid() const;

// qrc 路径格式化
static QString normalizedQrcFilePath(const QString &path);
static QString normalizedQrcDirectoryPath(const QString &path);
static QString qrcDirectoryPathForQrcFilePath(const QString &file);
```

```cpp
#include <QLocale>
#include <QString>
#include <QDebug>

#include <utils/qrcparser.h>

int main(){
    auto p = Utils::QrcParser::parseQrcFile("E:/simple.qrc", QString());

    QStringList lst;
    p->collectFilesAtPath("/cut.jpg", &lst);
    // QList("E:/workspace/qt servitor/tools/src/preview/test/assets/cut.jpg", 
    // "E:/workspace/qt servitor/tools/src/preview/test/assets/cut_fr.jpg")
    qDebug() << lst;

    QMap<QString, QStringList> map;
    p->collectFilesInPath("/", &map, true);
    // QList("cut.jpg", "images/", "myresources/")
    qDebug() << map.keys();

    // QList("", "fr")
    qDebug() << p->languages();

    QString content = R"(
    <qresource>
        <file>images/copy.png</file
        <file>images/paste.png</file>
        <file>images/save.png</file>
        <file>cut.jpg</file>
    </qresource>
    )";
    auto p1 = Utils::QrcParser::parseQrcFile("e:/test/test.qrc", content);

    // QList("XML error on line 4, col 13: Expected '>', but got '<'.")
    qDebug() << p1->errorMessages();
    return 0;
}
```

## QrcCache

一个 QrcParser 管理器，线程安全

```cpp
class QTCREATOR_UTILS_EXPORT QrcCache
{
public:
    QrcCache();
    ~QrcCache();
    // 添加 qrc
    QrcParser::ConstPtr addPath(const QString &path, const QString &contents);
    // 移除 qrc
    void removePath(const QString &path);
    // 更新 qrc
    QrcParser::ConstPtr updatePath(const QString &path, const QString &contents);
    // 解析
    QrcParser::ConstPtr parsedPath(const QString &path);
    void clear();
private:
    Internal::QrcCachePrivate *d;
};
```

# 调试器

> [调试服务](https://doc.qt.io/qt-5/qtquick-debugging.html#qml-debugging-infrastructure)

## 启动调试服务


```cmake
# 程序全局定义 QT_QML_DEBUG 与 QT_DECLARATIVE_DEBUG 宏，编译得到的程序便会启动 debugger 服务
add_definitions(-DQT_DECLARATIVE_DEBUG)
add_definitions(-DQT_QML_DEBUG)
```

```term
triangle@LEARN:~$ ./out -qmljsdebugger=file:xx.socket,block // 默认启动所有调试服务 
```


## 调试器客户端

```cpp

/* 自定义调试器 */
class  TranslationClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    using Ptr = QPointer<TranslationClient>;

    /* 
        NOTE - DebugTranslation 是固定的调试器客户端名。
            不同的功能服务需要实现不同的客户端
                - debugger:  DebugMessages,QmlDebugger,V8Debugger,QmlInspector,DebugTranslation
                - profiler: CanvasFrameRate,EngineControl,DebugMessages,DebugTranslation
                - Preview: QmlPreview,DebugTranslation
                - NativeDebugger: NativeQmlDebugger,DebugTranslation
     */
    TranslationClient::TranslationClient(QmlDebug::QmlDebugConnection *connection)
        :QmlDebug::QmlDebugClient(QLatin1String("DebugTranslation"), connection)
    {
    }

    /* 
        不同客户端对应不同功能以及通信协议，详情查看 qt creator 源码
     */
};


/* 调试器客户端管理器 */
class CustomDebugManager : public QmlDebug::QmlDebugConnectionManager{

public:
    /* 连接调试服务，QUrl 支持 socket 与 tcp 两种方案 */
    void connectToServer(const QUrl &server);

protected:
    /* 自动创建和销毁客户端，使用客户端，只需启动 CustomDebugManager 即可 */
    virtual void createClients() override{
         m_client = new TranslationClient(connection());
    }
    virtual void destroyClients() override{
        if(m_client){
            m_client->deleteLater();
        }
    }

private:
    TranslationClient:Ptr m_client;
};
```

## 通信协议

通信请求可以通过 [debuggerprotocol](https://github.com/qt-creator/qt-creator/tree/master/src/plugins/debugger/debuggerprotocol.h) 进行组装。

- **运行**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "continue",
        "arguments" : { 
                        "stepaction" : <"in", "next" or "out">, // 都不填就是继续运行
                        "stepcount"  : <number of steps (default 1)>
                    }
    }
```

- **暂停** : 直接发送 `interrupt`

- **设置断点**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "setbreakpoint",
        "arguments" : { 
                        "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
                        "target"      : <function expression or script identification>
                        "line"        : <line in script or function>
                        "column"      : <character position within the line>
                        "enabled"     : <initial enabled state. True or false, default is true>
                        "condition"   : <string with break point condition>
                        "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
                    }
    }
```


- **异常断点**

```json
    { "seq"       : <number>,
        "type"      : "request",
        "command"   : "setexceptionbreak",
        "arguments" : { "type"    : <string: "all", or "uncaught">,
                        "enabled" : <optional bool: enables the break type if true>
                    }
    }
```

- **清除断点**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "clearbreakpoint",
        "arguments" : { "breakpoint" : <number of the break point to clear>
                    }
    }
```

- **修改断点状态**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "clearbreakpoint",
        "arguments" : { 
                        "breakpoint" : <number of the break point>,
                        "enabled" : <0 , 1>
                    }
    }
```

- **表达式计算**

```json
       { "seq"       : <number>,
         "type"      : "request",
         "command"   : "evaluate",
         "arguments" : { "expression"    : <expression to evaluate>,
                         "frame"         : <number>,
                         "global"        : <boolean>,
                         "disable_break" : <boolean>,
                         "context"       : <object id>
                       }
       }
```

```json
    {   "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "evaluate",
        "body"        : ...
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```


- **监控**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "lookup",
        "arguments" : { "handles"       : <array of handles>,
                        "includeSource" : <boolean indicating whether
                                            the source will be included when
                                            script objects are returned>,
                    }
    }

```

```json
    {   "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "lookup",
        "body"        : <array of serialized objects indexed using their handle>
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```

- **堆栈**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "backtrace",
        "arguments" : { "fromFrame" : <number>
                        "toFrame" : <number>
                        "bottom" : <boolean, set to true if the bottom of the
                            stack is requested>
                    }
    }
```

```json
    {   "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "backtrace",
        "body"        : { "fromFrame" : <number>
                        "toFrame" : <number>
                        "totalFrames" : <number>
                        "frames" : <array of frames - see frame request for details>
                        }
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```

- **局部变量**

```json
   { "seq"       : <number>,
     "type"      : "request",
     "command"   : "frame",
     "arguments" : { "number" : <frame number> }
   }
```

```json
    { "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "frame",
        "body"        : { "index"          : <frame number>,
                        "receiver"       : <frame receiver>,
                        "func"           : <function invoked>,
                        "script"         : <script for the function>,
                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
                                                value: <value of the argument>
                                            },
                                            ... <the array contains all the arguments>
                                            ],
                        "locals"         : [ { name: <name of the local variable>,
                                                value: <value of the local variable>
                                            },
                                            ... <the array contains all the locals>
                                            ],
                        "position"       : <source position>,
                        "line"           : <source line>,
                        "column"         : <source column within the line>,
                        "sourceLineText" : <text for current source line>,
                        "scopes"         : [ <array of scopes, see scope request below for format> ],

                        }
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```

- **区域**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "scope",
        "arguments" : { "number" : <scope number>
                        "frameNumber" : <frame number, optional uses selected
                                        frame if missing>
                    }
    }
```

```json
    {   "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "scope",
        "body"        : { "index"      : <index of this scope in the scope chain. Index 0 is the top scope
                                        and the global scope will always have the highest index for a
                                        frame>,
                        "frameIndex" : <index of the frame>,
                        "type"       : <type of the scope:
                                        0: Global
                                        1: Local
                                        2: With
                                        3: Closure
                                        4: Catch >,
                        "object"     : <the scope object defining the content of the scope.
                                        For local and closure scopes this is transient objects,
                                        which has a negative handle value>
                        }
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```

- **能力查询**

```json
    {   "seq"       : <number>,
        "type"      : "request",
        "command"   : "version",
        "arguments" :{} 
    }
```

```json
       { "seq"         : <number>,
         "type"        : "response",
         "request_seq" : <number>,
         "command"     : "backtrace",
         "body"        : { "UnpausedEvaluate" : bool
                           "ContextEvaluate" : bool
                           "ChangeBreakpoint" : bool
                         }
         "running"     : <is the VM running after sending this response>
         "success"     : true
       }
```

## 调试服务返回

### response

```json
    {   "seq"         : <number>,
        "type"        : "response",
        "request_seq" : <number>,
        "command"     : "command",
        "body"        : {} 
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```


### event

- **break**

```json
    {
        "type"        : "event",
        "event"       : "break",
        "body"        : {
                            "invocationText": "str",
                            "script": {
                                    "name": "name"
                                },
                            "sourceLineText": "line",
                            "breakpoints": []
                        } 
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```

- **exception**

```json
    {
        "type"        : "event",
        "event"       : "exception",
        "body"        : {
                            "sourceLine": "str",
                            "script": {
                                    "name": "name"
                                },
                            "exception": {},
                            "text": "message"
                        } 
        "running"     : <is the VM running after sending this response>
        "success"     : true
    }
```
