add_rules("mode.debug", "mode.release")


add_cxflags("/utf-8")
add_ldflags("/SUBSYSTEM:CONSOLE")

add_defines("QT_CREATOR")

add_includedirs("include/qtcreator/qmljs",
                "include/qtcreator/",
                "include/qtcreator/utils/mimetypes2",
                "include/qtcreator/3rdparty")

if(is_mode("debug")) then
    add_linkdirs("lib/qtcreator/debug_win/")
else 
    add_linkdirs("lib/qtcreator/release_win/")
end

-- add_runenvs("PATH","E:/workspace/qml/qt-creator/build/bin/Debug")

if(is_mode("debug")) then
    set_targetdir("bin/debug_win/")
else
    set_targetdir("bin/release_win/")
end 

target("demo")

   add_rules("qt.quickapp")

   add_files("example/demo/main.cpp",
             "example/demo/main.qrc",
             "example/asset.qrc"
             )            

   set_targetdir("bin/demo")

target("preview")
    -- add_defines("DOCT_TEST")
    add_rules("qt.console")
    add_frameworks("QtNetwork")

    -- 头
    add_includedirs("src/preview/",
                    "src/")

    -- 库
    add_links("QmlJS","Utils","LanguageUtils","QmlDebug")

    -- 文件
    add_files(
            "src/preview/**/*.h",
            "src/preview/**/*.cpp",
            "src/preview/*.h",
            "src/preview/*.cpp",
            "src/common/*.cpp")



target("qmllsp")
    add_rules("qt.console")

    add_frameworks("QtNetwork","QtConcurrent","QtGui")
    add_links("QmlJS","Utils","LanguageUtils")


    add_includedirs("src/qmllsp/",
                    "src/")
    add_files("src/qmllsp/*.cpp",
              "src/qmllsp/**/*.cpp",
              "src/qmllsp/**/*.h",
              "src/common/*.cpp")


target("debug")
    add_rules("qt.console")

    add_linkdirs("lib/qtcreator")
    add_links("QmlJS","Utils","LanguageUtils")



