#ifndef _EasyTcpServer_H_
#define _EasyTcpServer_H_

#include <iostream>
#include <vector>
#include "MessageHeader.hpp"

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <cstring>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif

class EasyTcpServer
{
public:
    EasyTcpServer()
    {
        
    }

    virtual ~EasyTcpServer()
    {
        Close();
    }

    SOCKET InitSocket()
    {
        if(_listenSock != INVALID_SOCKET) return _listenSock;

#ifdef _WIN32
        WORD version = MAKEWORD(2, 2);
        WSADATA data;
        WSAStartup(version, &data);
#endif
        _listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(INVALID_SOCKET == _listenSock)
        {
            printf("[InitSocket], SOCKET创建失败...\n");
        }
        else
        {
            printf("[InitSocket], SOCKET创建成功...\n");
        }

        return _listenSock;
    }

    int Bind(const char* ip, const unsigned short port)
    {
        if(_listenSock == INVALID_SOCKET)
        {
            InitSocket();
        }

        sockaddr_in servAddr {};
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);

        if(!ip)
        {
            servAddr.sin_addr.s_addr = INADDR_ANY;
        }
        else
        {
            servAddr.sin_addr.s_addr = inet_addr(ip);
        }

        if(SOCKET_ERROR == bind(_listenSock, (sockaddr*)&servAddr, sizeof(servAddr)))
        {
            printf("Sock=<%d> 绑定网络端口失败...\n", (int)_listenSock);
            Close();
            return -1;
        }
        else
        {
            printf("Sock=<%d> 绑定网络端口<%d>成功...\n", (int)_listenSock, port);
        }

        return 0;
    }

    int Listen(int backlog)
    {
        if(SOCKET_ERROR == listen(_listenSock, backlog))
        {
            printf("Sock=<%d> 监听网络端口失败...\n", (int)_listenSock);
            Close();
            return -1;
        }
        else
        {
            printf("Sock=<%d> 监听网络端口成功...\n", (int)_listenSock);
        }

        return 0;
    }

    int Accept()
    {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(sockaddr_in);

#ifdef _WIN32
        SOCKET clientSock = accept(_listenSock, (sockaddr*)&clientAddr, &addrLen);
#else
        SOCKET clientSock = accept(_listenSock, (sockaddr*)&clientAddr, (socklen_t*)&addrLen);
#endif
        if(INVALID_SOCKET == clientSock)
        {
            printf("Sock=<%d> Accept客户端失败...\n", (int)_listenSock);

        }
        else
        {
            //群发消息通知玩家登录
            NewUserJoin msg;
            msg.newUserSocket = (int)clientSock;
            SendDataToAll(&msg);

            printf("Sock=<%d> 新客户端登录Socket=%d Ip=%s\n",(int)_listenSock, (int)clientSock, inet_ntoa(clientAddr.sin_addr));
            clients.emplace_back(clientSock);
        }     

        return (int)clientSock;
    }

    void Close()
    {
        if(_listenSock == INVALID_SOCKET) return;

#ifdef _WIN32
        //关闭所有套接字
        for(int n = (int)clients.size() - 1; n >= 0; n--)
        {
            closesocket(clients[n]);
        }
        closesocket(_listenSock);
        WSACleanup();
#else
        for(int n = (int)clients.size() - 1; n >= 0; n--)
        {
            close(clients[n]);
        }
        close(_listenSock);
#endif
    }


    int _ncount = 0;
    bool OnRun()
    {
        if(!isRun()) false;

        fd_set fdRead;
        fd_set fdWrite;
        fd_set fdError;

        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdError);

        FD_SET(_listenSock, &fdRead);
        FD_SET(_listenSock, &fdWrite);
        FD_SET(_listenSock, &fdError);
        SOCKET maxSock = _listenSock;

        for(int n = (int)clients.size() - 1; n >= 0; n--)
        {
            FD_SET(clients[n], &fdRead);
            maxSock = std::max<int>((int)maxSock, (int)clients[n]);
        }

        timeval tv { 1, 0 };
        int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdError, &tv);
        //printf("select ret = %d count=%d\n", ret, _ncount++);
        if(ret < 0)
        {
            printf("select任务结束!\n");
            Close();
            return false;
        }

        //检测客户端连接
        if(FD_ISSET(_listenSock, &fdRead))
        {
            FD_CLR(_listenSock, &fdRead);
            Accept();
        }

        auto it = clients.begin();
        while(it != clients.end())
        {
            if(FD_ISSET(*it, &fdRead))
            {
                if(-1 == ReceiveData(*it))
                {
                    it = clients.erase(it);
                    continue;
                }
            }
            ++it;
        }  

        //printf("空间时间处理其他业务...\n");

        return true;
    }

    bool isRun()
    {
        return _listenSock != INVALID_SOCKET;
    }


    char recvBuf[409600] {}; //应用层缓冲区
    //接收数据[处理粘包 拆分包]
    int ReceiveData(SOCKET clientSock)
    {
        int nRecv = (int)recv(clientSock, (char*)recvBuf, 409600, 0);
        if(nRecv <= 0)
        {
            printf("客户端[%d]断开连接!\n", (int)clientSock);
            return -1;
        }
        printf("<Sock=%d> recvLen = %d\n", (int)clientSock, nRecv);
        LoginResult res;
        SendData(clientSock, &res);

        /*char buf[256]{ 0 };

        int nRecv = (int)recv(clientSock, (char*)buf, sizeof(DataHeader), 0);  
        if(nRecv <= 0)
        {
            printf("客户端[%d]断开连接!\n", (int)clientSock);
            return -1;
        }

        DataHeader* header = (DataHeader*)buf;
        printf("接收到客户端[%d]发送命令: %d 数据长度: %d\n",(int)clientSock, header->cmd, header->dataLength);
        nRecv = recv(clientSock, (char*)buf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

        OnNetMsg(clientSock, header);*/
        
        return 0;
    }

    //响应消息包
    virtual void OnNetMsg(SOCKET clientSock, DataHeader* header)
    {
        switch( header->cmd )
        {
        case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                printf("收到Login命令,长度=%hd, username: %s, password: %s\n", login->dataLength, login->userName, login->passWord);
                //暂时忽略验证过程
                LoginResult loginRes;
                SendData(clientSock, &loginRes);
            }
            break;
        case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                printf("收到Logout命令,长度=%d, username: %s\n", logout->dataLength, logout->userName);
                //暂时忽略验证过程
                LogoutResult logoutRes;
                SendData(clientSock, &logoutRes);
            }
            break;
        default:
            {
                printf("receive unknown cmd!\n");
                ErrorResult errorRes;
                SendData(clientSock, &errorRes);
            }       
            break;
        }
    }

    //发送指定客户端
    int SendData(SOCKET clientSock, DataHeader* header)
    {
        if(isRun() && header)
        {
            return send(clientSock, (char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

    //发送到所有客户端
    void SendDataToAll(DataHeader* header)
    {
        if(isRun() && header)
        {
            for(int n = (int)clients.size() - 1; n >= 0; n--)
            {
                SendData(clients[n], header);
            }
        }
    }

private:
    SOCKET _listenSock = INVALID_SOCKET;
    std::vector<SOCKET> clients;
};

#endif
