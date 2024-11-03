#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <string>
#include <stdbool.h>

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#endif

#include "common/lspLog.h"

namespace tcp
{
    
typedef int socket_t;


struct SOCKET_INFO_S{
    sockaddr_in addr;
	socket_t fd = -1;
};


bool InitSocketNet();

bool CloseSocketNet();

socket_t InitSocket();

int CloseSocket(socket_t fd);

socket_t InitServer(const char * ip, uint32_t port);

SOCKET_INFO_S AcceptClientTimeOut(socket_t fd, int timeOut);
SOCKET_INFO_S AcceptClient(socket_t fd);

int ConnectServer(socket_t fd, const char * ip, uint32_t port);

int SendMsg(int fd, const char* buf, int len, int timeOut);

bool RecvMsg(int fd, char* buf, int bufSize, int timeOut);

// 接收数据，直到 strEnd 退出
bool RecvMsg(int fd, const std::string & strEnd, std::string & out, int timeOut);

} // namespace TCP



#endif /* SOCKET_H */