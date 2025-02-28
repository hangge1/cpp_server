#ifndef _EasyTcpClient_H_
#define _EasyTcpClient_H_

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
        Close();
    }

    SOCKET InitSocket()
    {
        if(_sock != INVALID_SOCKET) return _sock;

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

        return _sock;
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
            printf("<sock=%d> Connect: %s:%d 失败...\n", (int)_sock, ip, port);
            Close();
            return -1;
        }
        else
        {
            //printf("<sock=%d> Connect: %s:%d 成功...\n", (int)_sock, ip, port);
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
        close(_sock);
#endif
        _sock = INVALID_SOCKET;
    }

    int _ncount = 0;
    bool OnRun()
    {
        if(!isRun()) false;

        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(_sock, &fdReads);

        timeval tv {1, 0};
        int ret = select((int)_sock + 1, &fdReads, nullptr, nullptr, &tv);
        //printf("select ret = %d count=%d\n", ret, _ncount++);
        if(ret < 0)
        {
            //printf("<sock=%d> select任务结束\n",(int)_sock);
            Close();
            return false;
        }

        if(FD_ISSET(_sock, &fdReads))
        {
            FD_CLR(_sock, &fdReads);

            if(-1 == ReceiveData(_sock))
            {
                printf("<sock=%d> ReceiveData结束\n",(int)_sock);
                Close();
                return false;
            } 
        }

        return true;
    }

    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

#define RECV_BUFF_SIZE 10240
    //接收缓冲区
    char _recvBuf[RECV_BUFF_SIZE] {};
    //消息缓冲区(第二缓冲区)
    char _msgBuf[RECV_BUFF_SIZE*10] {};
    int _lastPos = 0;

    //接收数据[处理粘包 拆分包]
    int ReceiveData(SOCKET clientSock)
    {
        int nRecv = recv(clientSock, (char*)_recvBuf, RECV_BUFF_SIZE, 0);
        if(nRecv <= 0)
        {
            printf("<sock=%d> 与服务器断开连接!\n", (int)clientSock);
            return -1;
        }
        //printf("<Sock=%d> recvLen = %d\n", (int)clientSock, nRecv);
        
        memcpy(_msgBuf + _lastPos, _recvBuf, nRecv);
        _lastPos += nRecv;
        while (_lastPos > sizeof(DataHeader))
        {
            DataHeader* header = (DataHeader*)_msgBuf;
            int packageSize = header->dataLength;
            if(_lastPos >= packageSize) //够一个包长度
            {
                OnNetMsg(header); //处理包
                memmove(_msgBuf, _msgBuf + packageSize, _lastPos - packageSize); //前移
                _lastPos -= packageSize;
            }
            else break; //不够一个包长度
        }
        
        return 0;
    }

    //响应消息包
    virtual void OnNetMsg(DataHeader* header)
    {
        switch( header->cmd )
        {
        case CMD_LOGIN_RES:
            {
                LoginResult* login_res = (LoginResult*)header;
                //printf("<sock=%d> 收到服务器消息: CMD_LOGIN_RES, 数据长度: %d\n",(int)_sock, login_res->dataLength);
            }
            break;
        case CMD_LOGOUT_RES:
            {
                LogoutResult* logout_res = (LogoutResult*)header;
                //printf("<sock=%d> 收到服务器消息: CMD_LOGOUT_RES, 数据长度: %d\n",(int)_sock,  logout_res->dataLength);
            }
            break;
        case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* newJoin = (NewUserJoin*)header;
                //printf("<sock=%d> 收到服务器消息: CMD_NEW_USER_JOIN, 数据长度: %d\n",(int)_sock,  newJoin->dataLength);
            }
            break;
        case CMD_ERROR:
            {
                ErrorResult* err = (ErrorResult*)header;
                printf("<sock=%d> 收到服务器消息: CMD_ERROR, 数据长度: %d\n",(int)_sock,  err->dataLength);
            }
            break;
        default:
            {
                printf("<sock=%d> 收到服务器未知消息\n", (int)_sock);
            }
            break;
        }
    }

    int SendData(DataHeader* header)
    {
        int ret = SOCKET_ERROR;
        if(isRun() && header)
        {
            ret = send(_sock, (char*)header, header->dataLength, 0);
            if(ret == SOCKET_ERROR)
            {
                Close();
            }
        }
        return ret; 
    }

    int SendData(DataHeader* header, int nLen)
    {
        int ret = SOCKET_ERROR;
        if(isRun() && header)
        {
            ret = send(_sock, (char*)header, nLen, 0);
            if(ret == SOCKET_ERROR)
            {
                Close();
            }
        }
        return ret;
    }
private:
    SOCKET _sock = INVALID_SOCKET;
};

#endif