#ifndef EXITTASK_H
#define EXITTASK_H

#include "handleInterface/handler.h"

class ShutdownTask : public Handler{
protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};


class ExitTask : public Handler{
public:
    /* 处理请求 */
    virtual bool handleNotification(const Json & req) override;
};

#endif // EXITTASK_H
