#pragma once

#include <iostream>
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


class EasyTcpClient
{
public:
    EasyTcpClient()
    {

    }

    virtual ~EasyTcpClient()
    {
    
    }

    void InitSocket()
    {
        if(_sock != INVALID_SOCKET) return;

#ifdef _WIN32
        WORD version = MAKEWORD(2, 2);
        WSADATA data;
        WSAStartup(version, &data);
#endif
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(INVALID_SOCKET == _sock)
        {
            printf("[InitSocket], SOCKET创建失败...\n");
        }
        else
        {
            printf("[InitSocket], SOCKET创建成功...\n");
        }
    }

    int Connect(const char* ip, const unsigned short port)
    {
        if(INVALID_SOCKET == _sock)
        {
            InitSocket();
        }

        sockaddr_in servAddr{};
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);
        servAddr.sin_addr.s_addr = inet_addr(ip);

        if(SOCKET_ERROR == connect(_sock, (sockaddr*)&servAddr, sizeof(servAddr)))
        {
            printf("Connect失败...\n");
            return -1;
        }
        return 0;
    }

    void Close()
    {
        if(_sock == INVALID_SOCKET) return;

#ifdef _WIN32
        closesocket(_sock);
        WSACleanup();
#else
        close(clientSock);
#endif
        _sock = INVALID_SOCKET;
    }

    bool OnRun()
    {
        if(!isRun()) false;

        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(_sock, &fdReads);

        timeval tv {1, 0};
        int ret = select((int)_sock + 1, &fdReads, nullptr, nullptr, &tv);
        if(ret < 0)
        {
            printf("select任务结束\n");
            return false;
        }

        if(FD_ISSET(_sock, &fdReads))
        {
            FD_CLR(_sock, &fdReads);

            if(-1 == ReceiveData(_sock))
            {
                printf("2select任务结束\n");
                return false;
            } 
        }

        return true;
    }

    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //接收数据[处理粘包 拆分包]
    int ReceiveData(SOCKET clientSock)
    {
        char buf[256] { 0 };

        int nRecv = recv(clientSock, (char*)buf, sizeof(DataHeader), 0);
        if(nRecv <= 0)
        {
            printf("与服务器断开连接!\n");
            return -1;
        }
        
        DataHeader* header = (DataHeader*)buf;
        OnNetMsg(header);
        
        return 0;
    }

    //响应消息包
    void OnNetMsg(DataHeader* header)
    {
        switch( header->cmd )
        {
        case CMD_LOGIN_RES:
            {
                LoginResult* login_res = (LoginResult*)header;
                recv(_sock, (char*)login_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                printf("收到服务器消息: CMD_LOGIN_RES, 数据长度: %d\n", login_res->dataLength);
            }
            break;
        case CMD_LOGOUT_RES:
            {
                LogoutResult* logout_res = (LogoutResult*)header;
                recv(_sock, (char*)logout_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                printf("收到服务器消息: CMD_LOGOUT_RES, 数据长度: %d\n", logout_res->dataLength);
            }
            break;
        case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* newJoin = (NewUserJoin*)header;
                recv(_sock, (char*)newJoin + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                printf("收到服务器消息: CMD_NEW_USER_JOIN, 数据长度: %d\n", newJoin->dataLength);
            }
            break;
        }
    }

    int SendData(DataHeader* header)
    {
        if(isRun() && header)
        {
            return send(_sock, (char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }
private:
    SOCKET _sock = INVALID_SOCKET;
};