#ifndef COLORTASK_H
#define COLORTASK_H

#include "handleInterface/handler.h"

class ColorPresentationTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;

    /* 中断处理 */
    virtual bool handleInterrupt(const Json & req, Json & resp);
};

class DocumentColorTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};

#endif // COLORTASK_H
