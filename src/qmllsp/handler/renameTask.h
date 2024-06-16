#ifndef RENAMETASK_H
#define RENAMETASK_H

#include "handleInterface/handler.h"

class RenameTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};
#endif // RENAMETASK_H
