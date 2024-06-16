#ifndef DOCUMENTTASK_H
#define DOCUMENTTASK_H

#include "handleInterface/handler.h"

class DocumentOpenedTask : public Handler{

public:
    /* 处理请求 */
    virtual bool handleNotification(const Json & req) override;
};

class DocumentChangedTask : public Handler{

public:
    /* 处理请求 */
    virtual bool handleNotification(const Json & req) override;
};

class DocumentSavedTask : public Handler{

public:
    /* 处理请求 */
    virtual bool handleNotification(const Json & req) override;
};

#endif // DOCUMENTTASK_H
