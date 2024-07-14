#ifndef CONFIGTASK_H
#define CONFIGTASK_H

#include "handleInterface/handler.h"

class ConfigTask : public Handler{

public:
    /* 处理请求 */
    virtual bool handleNotification(const Json & req) override;
};

#endif // CONFIGTASK_H
