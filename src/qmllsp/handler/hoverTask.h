#ifndef HOVERTASK_H
#define HOVERTASK_H

#include "handleInterface/handler.h"

class HoverTask : public Handler{
protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;

};


#endif // HOVERTASK_H
