#ifndef PREVIEWLOG_H
#define PREVIEWLOG_H

#include <stdio.h>
#include <memory>
#include <string>
#include <direct.h>
#include <atomic>

#include "spdlog/spdlog.h"
#include "common/utils.h"
#include "common/singleton.hpp"


#ifdef __linux__
#define _FILE_NAME_   (strrchr(__FILE__, '/') + 1)
#else
#define _FILE_NAME_   (strrchr(__FILE__, '\\') + 1)
#endif


// 神奇的控制台指令
#define ARG_SPACE(n)  n,""
#define FORMAT_CLEAR   "\033c"
#define FORMAT_CURSOR_UP(n) "\r\033[" #n "A"
#define FORMAT_SPACE  "%*s"

#define LOGGER_NAME "logger"

// #ifdef DOC_TEST
// #define LOG_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_DEBUG(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_WARN(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #define CONSOLE_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_WARN(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_DEBUG(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #else
/* text 为 utf8 格式 */
#define LOG_ERROR(text,...)    if(OwO::Logger::Instance()->getLog()){spdlog::get(LOGGER_NAME)->log(spdlog::level::err, fmt::format("[{}:{}] " text,  _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__));}
#define LOG_DEBUG(text,...)    if(OwO::Logger::Instance()->getLog()){spdlog::get(LOGGER_NAME)->log(spdlog::level::debug, fmt::format("[{}:{}] " text,  _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__));}
#define LOG_WARN(text,...)     if(OwO::Logger::Instance()->getLog()){spdlog::get(LOGGER_NAME)->log(spdlog::level::warn, fmt::format("[{}:{}] " text,  _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__));}

/* text 为 utf8 格式 */
#define CONSOLE_ERROR(text,...) LOG_ERROR(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console(OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__)))); 
#define CONSOLE_WARN(text,...)  LOG_WARN(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console( OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__)))); 
#define CONSOLE_DEBUG(text,...) LOG_DEBUG(text,##__VA_ARGS__); \
                                OwO::Logger::Instance()->console(OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__)))); 

/* text 为 utf8 格式 */
#define INTERFACE_DEBUG(text,...)   LOG_ERROR(text,##__VA_ARGS__); \
                                    OwO::Logger::Instance()->interface(OwO::ToStdString(OwO::Utf8ToQString( OwO::Formats(text, ##__VA_ARGS__))))


#define ASSERT_RETURN(cond, msg, ...) \
    if((cond) != true){ \
        LOG_DEBUG(msg); \
        return __VA_ARGS__; \
    }
    // 

namespace OwO
{

class Logger : public Singleton<Logger>{

public:
    void console(const std::string & str);
    void interface(const std::string & str);

    void init(const std::string & strPath, bool bLog, bool bConsole);
    void setNeedNewLine(bool bFlag){ m_needNewLine.store(bFlag); }
    bool getNeedNewLine() { return m_needNewLine.load();}

    bool getLog(){return m_bLog;}
private:
    bool m_bLog;
    bool m_bConsole;
    std::string m_logFile;
    std::atomic_bool m_needNewLine;
};



} // namespace OwO


#endif /* PREVIEWLOG_H */
