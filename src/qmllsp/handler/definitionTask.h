#ifndef DEFINITIONTASK_H
#define DEFINITIONTASK_H

#include "handleInterface/handler.h"

class DefinitionTask : public Handler{

protected:
    virtual bool handleMessage(const Json & req, Json & resp) override;
};

#endif // DEFINITIONTASK_H
