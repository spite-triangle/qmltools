#ifndef COMPLETIONTASK_H
#define COMPLETIONTASK_H


#include "handleInterface/handler.h"

class CompletionTask: public Handler{

protected:
    
    /* 处理请求 */
    virtual bool handleMessage(const Json & req, Json & resp) override;

};

#endif // COMPLETIONTASK_H
