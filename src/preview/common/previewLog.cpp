#include "previewLog.h"

#include <iostream>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"


void OwO::Logger::console(const std::string &str)
{
    if(m_bConsole == false) return;

    printf(FORMAT_SPACE "\r%s\n>> ", ARG_SPACE(4) ,str.c_str());
    m_needNewLine.store(true);
}

void OwO::Logger::interface(const std::string & str){
    if(str.empty()){
        printf(">> ");
    }else{
        printf(FORMAT_SPACE "\r%s\n>> ", ARG_SPACE(10) ,str.c_str());
    }
}

void OwO::Logger::init(const std::string &strPath, bool bLog, bool bConsole)
{
    m_bLog = bLog;
    m_bConsole = bConsole;
    m_needNewLine.store(false);
    if(bLog == false) return;

    try 
    {
        auto logger = spdlog::rotating_logger_mt(LOGGER_NAME, strPath, 1024 * 1024 * 5, 5);
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l]%v");
        logger->flush_on(spdlog::level::debug);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}
