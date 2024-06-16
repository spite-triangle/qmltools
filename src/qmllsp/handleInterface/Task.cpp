
#include "task.h"

#include <QtConcurrent>



void Task::terminal()
{
    if(stop() == false) return;

    if(m_resTask.isValid()){
        m_resTask.waitForFinished();
    }
}


bool Task::run(const JsonPtr & req, const JsonPtr & resp)
{
    return m_handler->run(req, resp);
}


bool Task::stop()
{
    return m_handler->stop();
}


/* ===================================== */

TaskMessage::TaskMessage(const QString &id, const QString &method, const Handler::Ptr &handler)
{
    m_handler = handler;
    m_context.id = id;
    m_context.method = method;
}

void TaskMessage::distribute(const JsonPtr &req, Callback_t &&fcn)
{
    m_context.req = req;
    m_context.resp = std::make_shared<Json>();

    auto fcnRun = [this](const Callback_t &fcn){
            return fcn( m_context.id , run( m_context.req, m_context.resp));
        };

    m_resTask = QtConcurrent::run(fcnRun, std::move(fcn));
}

/* ===================================== */

TaskNotification::TaskNotification(const QString &method, const Handler::Ptr &handler)
{
    m_handler = handler;
    m_context.method = method;
}

void TaskNotification::distribute(const JsonPtr &req, Callback_t &&fcn)
{
    auto hanlder = m_handler; // 为了让 lambda 表达式持有该指针
    auto method = m_context.method;

    auto fcnRun = [=](){
        if((bool) fcn == true){
            return fcn(method, hanlder->run(req));
        }else{
            return hanlder->run(req);
        }
    };

    QtConcurrent::run(fcnRun);
}
