
#include "task.h"

#include <QtConcurrent>

Task::Task(const QString &id, const QString &method, const Handler::Ptr &handler)
{
    m_handler = handler;
    m_context.id = id;
    m_context.method = method;
}


void Task::distributePost(const JsonObjectPtr & req, Callback_t &&fcn)
{
    m_context.req = req;
    m_context.resp = std::make_shared<QJsonObject>();

    auto fcnRun = [this](const Callback_t &fcn){
            return fcn( m_context.id , run( m_context.req, m_context.resp));
        };

    m_resTask = QtConcurrent::run(fcnRun, std::move(fcn));
}

void Task::distributeMessage(const JsonObjectPtr &req, Callback_t &&fcn)
{
    m_context.req = req;

    auto fcnRun = [this](const Callback_t &fcn){
        if((bool)fcn == true){
            return fcn(m_context.id , run( m_context.req));
        }
        return run(m_context.req);
    };

    m_resTask = QtConcurrent::run(fcnRun, std::move(fcn));
}

void Task::terminal()
{
    if(stop() == false) return;

    if(m_resTask.isValid()){
        m_resTask.waitForFinished();
    }
}


bool Task::run(const JsonObjectPtr & req, const JsonObjectPtr & resp)
{
    return m_handler->run(req, resp);
}

bool Task::run(const JsonObjectPtr & req)
{
    return m_handler->run(req);
}

bool Task::stop()
{
    return m_handler->stop();
}
