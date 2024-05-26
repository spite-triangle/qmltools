
#include "task.h"

#include <QtConcurrent>

Task::Task(const QString &id, const QString &method, const Handler::Ptr &handler)
{
    m_handler = handler;
    m_context.id = id;
    m_context.method = method;
}

Task::Ptr Task::makeTask(const QString &id, const QString &method, const Handler::Ptr &handler)
{
    return std::make_shared<Task>(id, method, handler);
}

void Task::distribute(const JsonObjectPtr &req)
{
    m_context.req = req;
    m_context.resp = std::make_shared<QJsonObject>();
    m_resTask = QtConcurrent::run(&Task::run, this, m_context.req, m_context.resp);
}

void Task::distribute(const JsonObjectPtr & req, Callback_t &&fcn)
{
    m_context.req = req;
    m_context.resp = std::make_shared<QJsonObject>();

    auto fcnRun = [this](const JsonObjectPtr & req, const JsonObjectPtr & resp, Callback_t &&fcn){
            return fcn( m_context.id ,run(req, resp));
        };

    m_resTask = QtConcurrent::run(fcnRun, m_context.req, m_context.resp, std::move(fcn));
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

bool Task::stop()
{
    return m_handler->stop();
}
