#ifndef TASK_H
#define TASK_H


#include <QFuture>
#include <QString>
#include <QJsonObject>

#include <memory>
#include <functional>

#include "common/utils.h"
#include "common/lspDefine.h"
#include "handler/handler.h"

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

    static Task::Ptr makeTask(const QString & id, const QString & method, const Handler::Ptr & handler);

    /* 将任务推送到线程池 */
    void distribute(const JsonObjectPtr & req, Callback_t && fcn);

    /* 发出停止信号，并等待任务结束 */
    void terminal();

    FUNC_SET(Handler::Ptr, m_handler, Handler);
    FUNC_SET_GET(QString, m_context.id, Id);
    FUNC_SET_GET(QString, m_context.method, Method);
    FUNC_GET(JsonObjectPtr, m_context.resp, ResponsePtr);

protected:

    /* 线程池入口 */
    virtual bool run(const JsonObjectPtr & req, const JsonObjectPtr & resp);

    /* 停止信号 */
    virtual bool stop();

private:
    QFuture<bool> m_resTask; // 用于等待任务结束
    Handler::Ptr m_handler;

    CONTEXT_S m_context;
};

#endif // TASK_H
