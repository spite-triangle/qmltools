#ifndef INITIALIZETASK_H
#define INITIALIZETASK_H

#include "handler/Task.h"
#include "handler/handler.h"


class InitializeHandler : public Handler{
protected:
    virtual bool handleRequest(const JsonObjectPtr & req, JsonObjectPtr  resp) override;
};


#endif // INITIALIZETASK_H
