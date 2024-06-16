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

public:
    virtual ~Task() = default;

    /* 将任务推送到线程池 */
    virtual void distribute(const JsonPtr & req, Callback_t && fcn = Callback_t()) = 0;

    /* 发出停止信号，并等待任务结束 */
    void terminal();

    FUNC_GET(Handler::Ptr, m_handler, Handler);

protected:

    /* 线程池入口 */
    virtual bool run(const JsonPtr & req, const JsonPtr & resp);

    /* 停止信号 */
    virtual bool stop();

protected:
    QFuture<bool> m_resTask; // 用于等待任务结束
    Handler::Ptr m_handler;
};


class TaskMessage : public Task{

public:
    struct CONTEXT_S{
        QString id;
        QString method;
        JsonPtr req;
        JsonPtr resp;
    };

public:
    TaskMessage(const QString & id, const QString & method, const Handler::Ptr & handler);

    FUNC_SET_GET(QString, m_context.id, Id);
    FUNC_SET_GET(QString, m_context.method, Method);
    FUNC_GET(JsonPtr, m_context.resp, ResponsePtr);

    virtual void distribute(const JsonPtr & req, Callback_t && fcn ) override;

private:
    CONTEXT_S m_context;
};


class TaskNotification : public Task{

public:
    struct CONTEXT_S{
        QString method;
        JsonPtr req;
    };


public:
    TaskNotification(const QString & method, const Handler::Ptr & handler);

    FUNC_SET_GET(QString, m_context.method, Method);

    virtual void distribute(const JsonPtr & req, Callback_t && fcn) override;

private:
    CONTEXT_S m_context;
};



// 任务创建工厂
class TaskFactory{
public:
    using Ptr = std::shared_ptr<TaskFactory>;
public:
    virtual Task::Ptr createMessageTask(const QString & id, const QString & method){
        return Task::Ptr();
    }

    virtual Task::Ptr createNotificationTask(const QString & method) {
        return Task::Ptr();
    };
};


template <class HandlerType>
class TaskFactoryTemplate : public TaskFactory{
    
public:
    virtual Task::Ptr createMessageTask(const QString & id, const QString & method) override {
        return std::make_shared<TaskMessage>(id, method, std::make_shared<HandlerType>());
    }

    virtual Task::Ptr createNotificationTask(const QString & method) override {
        return std::make_shared<TaskNotification>(method,std::make_shared<HandlerType>());
    }

    static Ptr makeFactory(){
        return std::make_shared<TaskFactoryTemplate<HandlerType>>();
    }
};

#endif // TASK_H
