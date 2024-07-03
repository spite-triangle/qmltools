#ifndef COMPLETIONTASK_H
#define COMPLETIONTASK_H


#include "handleInterface/handler.h"

class CompletionTask: public Handler{

protected:
    
    /* 处理请求 */
    virtual bool handleMessage(const Json & req, Json & resp) override;

    /* 中断处理 */
    virtual bool handleInterrupt(const Json & req, Json & resp);

};

#endif // COMPLETIONTASK_H
