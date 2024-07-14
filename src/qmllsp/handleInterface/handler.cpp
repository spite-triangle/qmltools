
#include "handler.h"

#include "common/lspException.hpp"
#include "common/jsonUtil.hpp"
#include "server/lspServer.h"

bool Handler::run(const JsonPtr & req, JsonPtr resp) 
try{
    return handleMessage(*req, *resp);
}
catch (const InterruptException &e)
{
    return handleInterrupt(*req, *resp);
}catch (const std::exception & e){
    Json error = JsonUtil::ResponseError(*req, Json{
        {"code", LSP_ERROR_E::SERVER_CANCELLED},
        {"message", e.what()}
    });

    m_server->sendMsg(std::make_shared<Json>(error));
    return true;
}

bool Handler::run(const JsonPtr &req)
try{
    return handleNotification(*req);
}
catch (const InterruptException &e)
{
    return handleInterrupt(*req);
}
catch (const std::exception & e){
    return true;
}


bool Handler::stop() {
    m_bInterrupt = true;
    if(m_fcnStop){
        m_fcnStop();
    }
    return true;
}

void Handler::registerStop(Handler::StopFcn_t &&fcn)
{
    m_fcnStop = std::move(fcn);
    if(m_fcnStop){
        if(m_bInterrupt == true) m_fcnStop();
    }
}

bool Handler::checkInterrupt() {
    if(m_bInterrupt == true){
        throw InterruptException("taks Interrupt");
    }
    return m_bInterrupt;
}
