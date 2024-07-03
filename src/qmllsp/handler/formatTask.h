#ifndef FORMATTASK_H
#define FORMATTASK_H

#include "handleInterface/handler.h"

class FormatTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};


#endif // FORMATTASK_H
