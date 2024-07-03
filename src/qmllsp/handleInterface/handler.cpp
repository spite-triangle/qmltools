
#include "handler.h"

#include "common/lspException.hpp"

bool Handler::run(const JsonPtr & req, JsonPtr resp) 
try{
    return handleMessage(*req, *resp);
}
catch (const InterruptException &e)
{
    return handleInterrupt(*req, *resp);
}

bool Handler::run(const JsonPtr &req)
try{
    return handleNotification(*req);
}
catch (const InterruptException &e)
{
    return handleInterrupt(*req);
}

bool Handler::stop() {
    m_bInterrupt = true;
    return true;
}

bool Handler::checkInterrupt() {
    if(m_bInterrupt == true){
        throw InterruptException("taks Interrupt");
    }
    return m_bInterrupt;
}
