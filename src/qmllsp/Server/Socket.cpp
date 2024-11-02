#include "socket.h"

#define err(errMsg) printf("[line:%d] %s failed code %d\n", __LINE__, errMsg, WSAGetLastError());

namespace tcp{


bool InitSocketNet()
{
#ifdef _WIN32
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		err("WSAStartup");
		return false;
	}
#endif
	return true;
}

bool CloseSocketNet()
{

#ifdef _WIN32
	if (0 != WSACleanup()) {
		err("WSACleanup");
		return false;
	}
#endif
	return true;
}

socket_t InitSocket()
{
    // 1. 创建空 socket
	// param1: 地址协议族 ipv4 ipv6
	// param2: 传输协议类型 流式套接字 数据报
	// param3: 使用具体的某个传输协议
	socket_t fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1)
	{
		err("socket");
		return -1;
	}
    return fd;
}

int CloseSocket(socket_t fd)
{
    if(fd < 0) return 1;

#ifdef _WIN32
    return closesocket(fd);
#endif

#ifdef __linux__
    return close(fd);
#endif

}

// 发送消息
int SendMsg(int fd, const char* buf, int len) {
	return send(fd, buf, len, 0);
}

// 接收消息
bool RecvMsg(int fd, char* buf, int totalLen) {
    int recvLen = 0;
    const int readMax = 1024; 

    while (recvLen < totalLen)
    {
        int buffLen = (totalLen - recvLen) > readMax ? readMax : totalLen - recvLen;
        int len = recv(fd, buf + recvLen, buffLen, 0);

        if (len > 0) {
            recvLen += len;
        }
        else if (len == 0)
        {
            return false;
        }
        else {
            perror("recv");
            return false;
        }

    }
    return true;
}

bool RecvMsg(int fd, const std::string &strEnd, std::string &out)
{

    while (true)
    {
        char ch;
        int len = recv(fd, &ch, 1, 0);
        if(len > 0){
            out.push_back(ch);
        }else if(len == 0){
            return false;
        }else{
            LOG_ERROR("recv");
            return false;
        }

        if(out.size() >= strEnd.size()){
            auto  substr = out.substr(out.size() - strEnd.size(), strEnd.size());
            if(substr == strEnd) break;
        }
    }
    return true;
}


socket_t InitServer(const char * ip, uint32_t port)
{
    // 1. 创建空 socket
	socket_t fd = InitSocket();
	if (fd == -1)
	{
		err("socket");
		return -1;
	}

	// 2. 给socket绑定ip地址和端口号
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &saddr.sin_addr);

	int ret = bind(fd, (sockaddr*)&saddr, sizeof(saddr));
	if (ret == -1) {
		err("bind");
		return -1;
	}

	//3. 监听
	ret = listen(fd, 10);
	if (ret == -1)
	{
		err("listen");
		return -1;
	}
    return fd;
}

SOCKET_INFO_S AcceptClientTimeOut(socket_t fd, int timeOut)
{
     SOCKET_INFO_S client;
	//委托内核去检测监听描述符的读缓冲区是个可读
    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(fd, &fdread);

    int ret = 0;
    struct timeval val;
    val.tv_sec = timeOut;
    val.tv_usec = 0;

    while (1)
    {
        ret = select(fd+1, &fdread, NULL, NULL, &val); //设置超时
        if (ret == 0 && errno == EINTR)  //信号中断再超时一次
        {
            continue;
        }

        break;
    }

    return ret > 0 ? AcceptClient(fd) : client;
}


SOCKET_INFO_S AcceptClient(socket_t fd)
{
    SOCKET_INFO_S client;
	int addrLen = sizeof(sockaddr_in);
    client.fd = accept(fd, (struct sockaddr*)&client.addr, &addrLen);

    char ip[32];
    LOG_ERROR("客户端fd: {}, IP：{}, 端口: {}\n",
        client.fd,
        inet_ntop(AF_INET, &client.addr.sin_addr.S_un, ip, sizeof(ip)),
        ntohs(client.addr.sin_port)
    );

    return client;
}

int ConnectServer(socket_t fd, const char * ip, uint32_t port)
{
	// 2. 与服务器建立连接
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	
	int ret = connect(fd, (sockaddr*)&addr, sizeof(addr));
	if ( ret == -1)
	{
		err("connect");
		return -1;
	}
    return ret;
}

}


