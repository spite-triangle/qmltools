#include "lspLog.h"

#include "iostream"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"


void OwO::Logger::init(const std::string &strPath, bool bLog)
{
    m_bLog = bLog;
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