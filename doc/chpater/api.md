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