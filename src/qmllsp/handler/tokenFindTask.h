#ifndef TOKENFINDTASK_H
#define TOKENFINDTASK_H

#include "handleInterface/handler.h"

class ReferencesFindTask : public Handler{
protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};


class DefineTask : public Handler{
protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};

#endif // TOKENFINDTASK_H
