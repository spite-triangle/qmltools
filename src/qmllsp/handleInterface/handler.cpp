
#include "handler.h"

#include "common/lspException.hpp"

bool Handler::run(const JsonPtr & req, JsonPtr resp) 
try{
    return handleMessage(*req, *resp);
}
catch (const InterruptException &e)
{
    return handleInterrupt();
}

bool Handler::run(const JsonPtr &req)
try{
    return handleNotification(*req);
}
catch (const InterruptException &e)
{
    return handleInterrupt();
}

bool Handler::stop() {
    m_bInterrupt = true;
    return true;
}

void Handler::checkInterrupt() {
    if(m_bInterrupt == true){
        throw InterruptException("taks Interrupt");
    }
}
