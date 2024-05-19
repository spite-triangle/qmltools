
#include "lspServer.h"

QByteArray LspServere::recv(QIODevice * io) {

    io->startTransaction();

    QByteArray datas = io->readAll();
    size_t len = recvDataLen(datas);

    // TODO -  校验 len 
    if(len){

        io->commitTransaction();
        return datas;
    }

    io->rollbackTransaction();
    return QByteArray();
}

void LspServere::send(QIODevice * io, const QByteArray & data) {
    const auto & total = data.length(); 

    size_t sendLen = 0;
    int len = 1024; // 一次发送的长度

    while (sendLen < total)
    {
        size_t diff = total - sendLen;
        len =  diff < len ?  diff : len;
        sendLen += io->write(data.data() + sendLen, len);
    }
}

size_t LspServere::recvDataLen(const QByteArray & bytes) {
    size_t returnValue;
    return returnValue;
}
