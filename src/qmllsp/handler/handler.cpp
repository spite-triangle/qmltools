
#include "handler.h"

#include "common/lspException.hpp"

bool Handler::run(const JsonObjectPtr & req, JsonObjectPtr resp) try{
    return handleRequest(req, resp);
}catch(const InterruptException & e){
    
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