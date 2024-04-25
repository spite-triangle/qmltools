#ifndef LOG_HPP
#define LOG_HPP

#include <stdio.h>
#include <memory>
#include <string>

#include "common/singleton.hpp"

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

    void init(const std::string & strPath, bool bLog){
        m_bLog = bLog;
        if(bLog == false) return;
        m_logFile = strPath;
    }

private:


private:
    bool m_bLog;
    std::string m_logFile;
};



} // namespace OwO




#endif /* LOG_HPP */
