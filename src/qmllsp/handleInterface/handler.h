#ifndef PROVIDER_H
#define PROVIDER_H

#include <memory>

#include <QJsonObject>

#include "common/utils.h"
#include "common/lspDefine.h"

class Handler{

public:
    using Ptr = std::shared_ptr<Handler>;

public:
    virtual ~Handler() = default;

    bool run(const JsonObjectPtr & req);
    bool run(const JsonObjectPtr & req, JsonObjectPtr resp);
    bool stop();
    
protected:
    /* 中断任务 */
    void checkInterrupt();

    /* 处理请求 */
    virtual bool handleRequest(const JsonObjectPtr & req, JsonObjectPtr resp){ return false;};

    /* 处理 message */
    virtual bool handleMessage(const JsonObjectPtr & req){ return false; };

    /* 中断处理 */
    virtual bool handleInterrupt(){ return true; };

private:
    bool m_bInterrupt;
};


#endif // PROVIDER_H
