#ifndef INITIALIZETASK_H
#define INITIALIZETASK_H

#include "handleInterface/Task.h"
#include "handleInterface/handler.h"


class InitializeHandler : public Handler{
protected:
    virtual bool handleRequest(const JsonObjectPtr & req, JsonObjectPtr  resp) override;
};

#endif // INITIALIZETASK_H
