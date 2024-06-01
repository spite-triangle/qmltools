#ifndef TASK_H
#define TASK_H


#include <QFuture>
#include <QString>
#include <QJsonObject>

#include <memory>
#include <functional>

#include "common/utils.h"
#include "common/lspDefine.h"
#include "handleInterface/handler.h"

class Task{
public:
    using Ptr = std::shared_ptr<Task>;
    using Callback_t = std::function<bool(const QString & id, bool)>;

    struct CONTEXT_S{
        QString id;
        QString method;
        JsonObjectPtr req;
        JsonObjectPtr resp;
    };

public:
    Task(const QString & id, const QString & method, const Handler::Ptr & handler);
    virtual ~Task() = default;

    /* 将任务推送到线程池 */
    void distributePost(const JsonObjectPtr & req, Callback_t && fcn);
    void distributeMessage(const JsonObjectPtr & req, Callback_t && fcn = Callback_t());

    /* 发出停止信号，并等待任务结束 */
    void terminal();

    FUNC_SET(Handler::Ptr, m_handler, Handler);
    FUNC_SET_GET(QString, m_context.id, Id);
    FUNC_SET_GET(QString, m_context.method, Method);
    FUNC_GET(JsonObjectPtr, m_context.resp, ResponsePtr);

protected:

    /* 线程池入口 */
    virtual bool run(const JsonObjectPtr & req, const JsonObjectPtr & resp);
    virtual bool run(const JsonObjectPtr & req);

    /* 停止信号 */
    virtual bool stop();

private:
    QFuture<bool> m_resTask; // 用于等待任务结束
    Handler::Ptr m_handler;

    CONTEXT_S m_context;
};


// 任务创建工厂
class TaskFactory{
public:
    using Ptr = std::shared_ptr<TaskFactory>;
public:
    virtual Task::Ptr createTask(const QString & id, const QString & method) = 0;
};


template <class Handler>
class TaskFactoryTemplate : public TaskFactory{

public:
    virtual Task::Ptr createTask(const QString & id, const QString & method){
        return std::make_shared<Task>(id, method, std::make_shared<Handler>());
    }

    static Ptr makeFactory(){
        return std::make_shared<TaskFactoryTemplate<Handler>>();
    }
};

#endif // TASK_H
