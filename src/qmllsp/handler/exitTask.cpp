#include "exitTask.h"

#include "common/jsonUtil.hpp"

#include <QCoreApplication>

bool ShutdownTask::handleMessage(const Json &req, Json &resp)
{
    resp = JsonUtil::ResponseMessge(req, Json());
    return true;
}

bool ExitTask::handleNotification(const Json &req)
{
    QCoreApplication::exit(0);
    return true;
}
