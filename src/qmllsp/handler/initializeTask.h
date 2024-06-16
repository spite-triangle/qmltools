#ifndef INITIALIZETASK_H
#define INITIALIZETASK_H

#include "handleInterface/handler.h"

class InitializeHandler : public Handler{
protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};


class InitializedHandler : public Handler{
protected:
    virtual bool handleNotification(const Json & req) override;
};

#endif // INITIALIZETASK_H
