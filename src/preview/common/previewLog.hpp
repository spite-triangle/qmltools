#ifndef PREVIEWLOG_HPP
#define PREVIEWLOG_HPP

#include <stdio.h>
#include <memory>
#include <string>

#include "common/singleton.hpp"
#include "common/previewProject.h"


#ifdef __linux__
#define _FILE_NAME_   (strrchr(__FILE__, '/') + 1)
#else
#define _FILE_NAME_   (strrchr(__FILE__, '\\') + 1)
#endif


// #ifdef DOC_TEST
// #define LOG_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_DEBUG(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_WARN(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #define CONSOLE_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_WARN(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_DEBUG(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #else
/* text 为 utf8 格式 */
#define LOG_ERROR(text,...)     OwO::Logger::Instance()->log("ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_DEBUG(text,...)     OwO::Logger::Instance()->log("DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_WARN(text,...)      OwO::Logger::Instance()->log("WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

/* text 为 utf8 格式 */
#define CONSOLE_ERROR(text,...) LOG_ERROR(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console( "%s\n", OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__))).c_str()); 
#define CONSOLE_WARN(text,...)  LOG_WARN(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console( "%s\n", OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__))).c_str()); 
#define CONSOLE_DEBUG(text,...) LOG_DEBUG(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console( "%s\n", OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__))).c_str()); 

/* text 为 utf8 格式 */
#define INTERFACE_DEBUG(text,...) LOG_ERROR(text,##__VA_ARGS__); printf("\r%s\n>> ", OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__))).c_str())



#define ASSERT_RETURN(cond, msg, ...) \
    if((cond) != true){ \
        LOG_DEBUG(msg); \
        return __VA_ARGS__; \
    }
    // 

#define RAII_DEFER(code) \
    std::shared_ptr<void> __raii(nullptr, [&](void*){\
        code;\
    })
    // 

namespace OwO
{

class Logger : public Singleton<Logger>{

public:

    template<class ... Args>
    void log(const char * format, Args ... args){
        if(m_logFile.empty() || m_bLog == false) return;

        FILE* file = nullptr;
#ifdef  __linux__
        file = fopen(m_logFile.c_str(), "a");
        fprintf(file, format, args...);
        fclose(file);
#else
        fopen_s(&file, m_logFile.c_str(), "a");
        if(file == nullptr) return;
        fprintf_s(file, format, args...);
        fclose(file);
#endif
    }

    template<class ... Args>
    void console(const char * format, Args ... args){
        if(ProjectExplorer::Project::Instance()->getConsoleLog() == false) return;
        printf(format, args...);
        /* 输入复位 */
        printf("\r>> ");
    }
    void init(const std::string & strPath, bool bLog){
        m_bLog = bLog;
        if(bLog == false) return;
        m_logFile = strPath;
    }

private:
    bool m_bLog;
    std::string m_logFile;
};



} // namespace OwO






#endif /* PREVIEWLOG_HPP */
