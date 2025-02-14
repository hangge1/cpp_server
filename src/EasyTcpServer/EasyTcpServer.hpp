#ifndef _EasyTcpServer_H_
#define _EasyTcpServer_H_

#include <iostream>
#include <vector>
#include <list>
#include "MessageHeader.hpp"
#include "TimeStamp.hpp"

#ifdef _WIN32
    #define FD_SETSIZE 1024
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
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

#ifndef RECV_BUFF_SIZE
    #define RECV_BUFF_SIZE 10240
#endif

class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
        : _sockfd(sockfd)
    {
        
    }

    SOCKET sockfd() { return _sockfd; }
    char* msgBuf() { return _msgBuf; }
    int lastPos() { return _lastPos; }
    void setLastPos(int lastPos) { _lastPos = lastPos; }
private:
    SOCKET _sockfd = INVALID_SOCKET;
    //消息缓冲区(第二缓冲区)
    char _msgBuf[RECV_BUFF_SIZE*10] {0};
    int _lastPos = 0;
};

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
            //群发消息通知其他玩家
            NewUserJoin msg;
            msg.newUserSocket = (int)clientSock;
            SendDataToAll(&msg);

            _clients.emplace_back(new ClientSocket(clientSock));
 
            printf("Sock=<%d> 新客户端<%d>[%d]加入 Ip=%s\n",(int)_listenSock, (int)clientSock, (int)_clients.size(), inet_ntoa(clientAddr.sin_addr));
        }     

        return (int)clientSock;
    }

    void Close()
    {
        if(_listenSock == INVALID_SOCKET) return;

#ifdef _WIN32
        //关闭所有套接字
        /*for(int n = (int)_clients.size() - 1; n >= 0; n--)
        {
            closesocket(_clients[n]->sockfd());
        }*/

        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            closesocket((*it)->sockfd());
        }

        closesocket(_listenSock);
        WSACleanup();
#else
       /* for(int n = (int)_clients.size() - 1; n >= 0; n--)
        {
            close(_clients[n]->sockfd());
        }*/

        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            close((*it)->sockfd());
        }

        close(_listenSock);
#endif
        _listenSock = INVALID_SOCKET;
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

        /*for(int n = (int)_clients.size() - 1; n >= 0; n--)
        {
            FD_SET(_clients[n]->sockfd(), &fdRead);
            maxSock = std::max<int>((int)maxSock, (int)_clients[n]->sockfd());
        }*/

        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            FD_SET((*it)->sockfd(), &fdRead);
            maxSock = std::max<int>((int)maxSock, (int)(*it)->sockfd());
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
            //FD_CLR(_listenSock, &fdRead);
            Accept();
        }

        auto it = _clients.begin();
        while(it != _clients.end())
        {
            if(FD_ISSET((*it)->sockfd(), &fdRead))
            {
                if(-1 == ReceiveData(*it))
                {
                    delete *it;
                    it = _clients.erase(it);
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


    //接收缓冲区
    char _recvBuf[RECV_BUFF_SIZE] {};
    //接收数据[处理粘包 拆分包]
    int ReceiveData(ClientSocket* pClientSock)
    {
        int nRecv = (int)recv(pClientSock->sockfd(), (char*)_recvBuf, RECV_BUFF_SIZE, 0);
        if(nRecv <= 0)
        {
            printf("客户端[%d]断开连接!\n", (int)pClientSock->sockfd());
            return -1;
        }
        //printf("<Sock=%d> recvLen = %d\n", (int)pClientSock->sockfd(), nRecv);
        
        memcpy(pClientSock->msgBuf() + pClientSock->lastPos(), _recvBuf, nRecv);
        pClientSock->setLastPos(pClientSock->lastPos() + nRecv);

        while (pClientSock->lastPos() > sizeof(DataHeader))
        {
            DataHeader* header = (DataHeader*)pClientSock->msgBuf();
            int packageSize = header->dataLength;
            int stillmsgSize = pClientSock->lastPos() - header->dataLength;
            if(pClientSock->lastPos() >= packageSize) //够一个包长度
            {
                OnNetMsg(pClientSock->sockfd(), header); //处理包
                memmove(pClientSock->msgBuf(), pClientSock->msgBuf() + packageSize, stillmsgSize); //前移
                pClientSock->setLastPos(stillmsgSize);
            }
            else break; //不够一个包长度
        }
        
        
        return 0;
    }

    //响应消息包
    virtual void OnNetMsg(SOCKET clientSock, DataHeader* header)
    {
        _recvCount++;
        double timeinterval = _timeStamp.getElapsedSecond();
        if(timeinterval >= 1.0)
        {
            printf("timeInterval=<%lf>, Sock=<%d>, recvPackCount=<%d>\n",timeinterval, (int)_listenSock, _recvCount);
            _timeStamp.Update();
            _recvCount = 0;
        }

        switch( header->cmd )
        {
        case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                //printf("收到客户端<%d>请求: CMD_LOGIN, 数据长度=%hd, username: %s\n",(int)clientSock, login->dataLength, login->userName);
                //暂时忽略验证过程
                //LoginResult loginRes;
                //SendData(clientSock, &loginRes);
            }
            break;
        case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                //printf("收到客户端<%d>请求: CMD_LOGOUT, 数据长度=%d, username: %s\n",(int)clientSock, logout->dataLength, logout->userName);
                //暂时忽略验证过程
                //LogoutResult logoutRes;
                //SendData(clientSock, &logoutRes);
            }
            break;
        default:
            {
                printf("收到客户端<%d>未知的请求! 数据长度=%d\n", (int)clientSock, header->dataLength);
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
            for(auto it = _clients.begin(); it != _clients.end(); ++it)
            {
                SendData((*it)->sockfd(), header);
            }
        }
    }

private:
    SOCKET _listenSock = INVALID_SOCKET;
    std::list<ClientSocket*> _clients;

    TimeStamp _timeStamp;
    int _recvCount = 0;
};

#endif
