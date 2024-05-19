#ifndef PREVIEWLOG_HPP
#define PREVIEWLOG_HPP

#include <stdio.h>
#include <memory>
#include <string>
#include <direct.h>
#include <atomic>

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


// #ifdef DOC_TEST
// #define LOG_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_DEBUG(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define LOG_WARN(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #define CONSOLE_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_WARN(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
// #define CONSOLE_DEBUG(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

// #else
/* text 为 utf8 格式 */
#define LOG_ERROR(text,...)     OwO::Logger::Instance()->log("ERROR [%s:%d] " text "\n",  _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_DEBUG(text,...)     OwO::Logger::Instance()->log("DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_WARN(text,...)      OwO::Logger::Instance()->log("WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

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

    void console(const std::string & str){
        if(m_bConsole == false) return;

        printf(FORMAT_SPACE "\r%s\n>> ", ARG_SPACE(4) ,str.c_str());
        m_needNewLine.store(true);
    }

    void interface(const std::string & str){
        if(str.empty()){
            printf(">> ");
        }else{
            printf(FORMAT_SPACE "\r%s\n>> ", ARG_SPACE(10) ,str.c_str());
        }
    }

    void init(const std::string & strPath, bool bLog, bool bConsole){
        m_bLog = bLog;
        m_bConsole = bConsole;
        m_needNewLine.store(false);

        if(bLog == false) return;
        m_logFile = strPath;

        // 创建文件夹
        std::string folder = strPath.substr(0,strPath.find_last_of('/'));
        if(OwO::MakeDirectory(folder) == false){
            CONSOLE_ERROR("failed to make log folder.");
            m_bLog = false;
        }
    }


    void setNeedNewLine(bool bFlag){ m_needNewLine.store(bFlag); }
    bool getNeedNewLine() { return m_needNewLine.load();}

private:
    bool m_bLog;
    bool m_bConsole;
    std::string m_logFile;
    std::atomic_bool m_needNewLine;
};



} // namespace OwO






#endif /* PREVIEWLOG_HPP */
