#ifndef TESTQRCPARSE_HPP
#define TESTQRCPARSE_HPP

#include "common/doctest.hpp"

#include <QLocale>
#include <QString>
#include <QDebug>

#include <utils/qrcparser.h>

TEST_CASE("Qrc Parser"){
    
    auto p = Utils::QrcParser::parseQrcFile("E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc", QString());

    SUBCASE("collectFiles"){
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
    } 

    SUBCASE("collectResourceFilesForSourceFile"){
        QStringList lst;
        p->collectResourceFilesForSourceFile("E:/workspace/qt servitor/tools/src/preview/test/assets/images/cut.png",&lst);
        qDebug() << lst;

        lst.clear();
        p->collectResourceFilesForSourceFile("E:/workspace/qt servitor/tools/src/preview/test/assets/images/cut1234.png",&lst);
        qDebug() << lst;
    }

    SUBCASE("firstFile"){
        auto str = p->firstFileAtPath("/cut.jpg", QLocale(QLatin1String("fr_FR")));
        str = p->firstFileAtPath("/cut.jpg", QLocale());
    }

    SUBCASE("normalize"){
        // "/E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc"
        qDebug() << Utils::QrcParser::normalizedQrcFilePath("E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc");

        // "/E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc/"
        qDebug() << Utils::QrcParser::normalizedQrcDirectoryPath("E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc");

        // "E:/workspace/qt servitor/tools/src/preview/test/assets"
        qDebug() << Utils::QrcParser::qrcDirectoryPathForQrcFilePath("E:/workspace/qt servitor/tools/src/preview/test/assets/simple.qrc");
    }

    SUBCASE("errorMessages"){
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
    }


}


#endif /* TESTQRCPARSE_HPP */
