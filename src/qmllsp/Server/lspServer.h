#ifndef LSPSERVER_H
#define LSPSERVER_H

#include <QObject>
#include <QIODevice>
#include <QByteArray>

class LspServere : public QObject{
    Q_OBJECT
public:
    LspServere(QObject * parent)
        : QObject(parent)
    {}

    /* 启动服务 */
    virtual bool start() = 0;
protected:
    // 数据接收发送
    QByteArray recv(QIODevice * io);
    void send(QIODevice * io, const QByteArray & data);

    // 解析数据大小
    size_t recvDataLen(const QByteArray & bytes);
};


#endif // LSPSERVER_H
