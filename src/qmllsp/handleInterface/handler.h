#ifndef PROVIDER_H
#define PROVIDER_H

#include <memory>
#include <functional>

#include <QJsonObject>

#include "common/utils.h"
#include "common/lspDefine.h"

class LspServer;

class Handler{

public:
    using Ptr = std::shared_ptr<Handler>;
    using StopFcn_t = std::function<void()>;

public:
    virtual ~Handler() = default;

    bool run(const JsonPtr & req);
    bool run(const JsonPtr & req, JsonPtr resp);
    bool stop();
    void registerStop(StopFcn_t && fcn);
    
    FUNC_SET(std::shared_ptr<LspServer>, m_server, Server);

protected:
    /* 中断任务 */
    bool checkInterrupt();

    /* 处理请求 */
    virtual bool handleMessage(const Json & req, Json & resp){ return false;};

    /* 处理 notification */
    virtual bool handleNotification(const Json & req){ return false; };

    /* 中断处理 */
    virtual bool handleInterrupt(const Json & req){ return true; };
    virtual bool handleInterrupt(const Json & req, Json & resp){ return true; };

protected:
    bool m_bInterrupt;
    std::function<void()> m_fcnStop;
    std::shared_ptr<LspServer> m_server;
};


#endif // PROVIDER_H
