#ifndef COLORTASK_H
#define COLORTASK_H

#include "handleInterface/handler.h"

class ColorPresentationTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};

class DocumentColorTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};

#endif // COLORTASK_H
