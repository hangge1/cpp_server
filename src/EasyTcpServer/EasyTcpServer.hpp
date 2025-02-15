#ifndef _EasyTcpServer_H_
#define _EasyTcpServer_H_

#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
#include <algorithm>

#include "MessageHeader.hpp"
#include "TimeStamp.hpp"

#ifdef _WIN32
    #define FD_SETSIZE 10024
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

const int kCellServer_Thread_Count = 4;

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


class INetEvent
{
public:
    //客户端断开连接
    virtual void OnLeave(ClientSocket* pClient) = 0; //纯虚函数 
};


class CellServer
{
public:
    CellServer(SOCKET listenSock = INVALID_SOCKET)
        : _listenSock(listenSock), _pThread(nullptr), RecvCount(0), _pNetEvent(nullptr)
    {
        
    } 

    virtual ~CellServer()
    {
        Close();
        RecvCount = 0;
    }

    void setNetEvent(INetEvent* pNetEvent)
    {
        _pNetEvent = pNetEvent;
    }

    bool OnRun()
    {
        while(isRun())
        {
            { //从缓冲队列获取新分配的客户端
                std::lock_guard<std::mutex> lock(_mutex);
                if(!_clientsBuff.empty())
                {
                    std::copy(_clientsBuff.begin(), _clientsBuff.end(), std::back_inserter(_clients));
                    _clientsBuff.clear();
                }
            }

            if(_clients.empty()) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            fd_set fdRead;
            FD_ZERO(&fdRead);

            SOCKET maxSock = (*_clients.begin())->sockfd();
            for(auto it = _clients.begin(); it != _clients.end(); ++it)
            {
                FD_SET((*it)->sockfd(), &fdRead);
                maxSock = std::max<int>((int)maxSock, (int)(*it)->sockfd());
            }

            //timeval tv { 1, 0 };
            int ret = select((int)maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
            if(ret < 0)
            {
                printf("select任务结束!\n");
                Close();
                return false;
            }

            auto it = _clients.begin(); 
            while(it != _clients.end()) 
            {
                if(FD_ISSET((*it)->sockfd(), &fdRead)) 
                {
                    if(-1 == ReceiveData(*it)) 
                    {
                        if(_pNetEvent) _pNetEvent->OnLeave(*it);
                        delete *it;
                        it = _clients.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
        
        return true;
    }

    bool isRun()
    {
        return _listenSock != INVALID_SOCKET;
    }

    void Close()
    {
        if(_listenSock == INVALID_SOCKET) return;

#ifdef _WIN32
        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            closesocket((*it)->sockfd());
        }

        closesocket(_listenSock);
        WSACleanup();
#else
        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            close((*it)->sockfd());
        }

        close(_listenSock);
#endif
        _listenSock = INVALID_SOCKET;
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
        RecvCount++;

        /*_recvCount++;
        double timeinterval = _timeStamp.getElapsedSecond();
        if(timeinterval >= 1.0)
        {
            printf("timeInterval=<%lf>, Sock=<%d>, CSock=<%d>, recvPackCount=<%d>\n",timeinterval, (int)_listenSock,  (int)clientSock, _recvCount);
            _timeStamp.Update();
            _recvCount = 0;
        }*/

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

    void AddClient(ClientSocket* pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(pClient)
        {
            _clientsBuff.push_back(pClient);
        }
    }

    size_t GetClientCount()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _clients.size() + _clientsBuff.size();
    }

    void Start()
    {
        if(_pThread) return;

        _pThread = new std::thread(&CellServer::OnRun, this);
    }
private:
    SOCKET _listenSock = INVALID_SOCKET;
    //所有通信客户端
    std::list<ClientSocket*> _clients;

    //新加入客户端缓冲队列(线程通信)
    std::vector<ClientSocket*> _clientsBuff;
    std::mutex _mutex;

    std::thread* _pThread;

    INetEvent* _pNetEvent;
public:
    std::atomic_int RecvCount;
};

class EasyTcpServer : public INetEvent
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

    void Start()
    {
        for(int i = 0; i < kCellServer_Thread_Count; i++)
        {
            CellServer* cell = new CellServer(_listenSock);
            cell->setNetEvent(this);
            _cellServers.push_back(cell);
            cell->Start();
        }
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
            /*NewUserJoin msg;
            msg.newUserSocket = (int)clientSock;
            SendDataToAll(&msg);*/

            AddClient2CellServer(new ClientSocket(clientSock));
 
            printf("Sock=<%d> 新客户端<%d>[%d]加入 Ip=%s\n",(int)_listenSock, (int)clientSock, (int)_clients.size(), inet_ntoa(clientAddr.sin_addr));
        }     

        return (int)clientSock;
    }

    void AddClient2CellServer(ClientSocket* pClientSock)
    {
        assert(_cellServers.size() > 0);

        _clients.push_back(pClientSock);

        size_t minCount = _cellServers[0]->GetClientCount();
        CellServer* pMinCountCellServer = _cellServers[0];
        for(auto* pCellServer : _cellServers)
        {
            size_t count = pCellServer->GetClientCount();
            if(count < minCount)
            {
                pMinCountCellServer = pCellServer;
                minCount = count;
            }
        }

        if(pMinCountCellServer)
        {
            pMinCountCellServer->AddClient(pClientSock);
        }
    }

    void Close()
    {
        if(_listenSock == INVALID_SOCKET) return;

#ifdef _WIN32
        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            closesocket((*it)->sockfd());
        }

        closesocket(_listenSock);
        WSACleanup();
#else
        for(auto it = _clients.begin(); it != _clients.end(); ++it)
        {
            close((*it)->sockfd());
        }

        close(_listenSock);
#endif
        _listenSock = INVALID_SOCKET;
    }

    bool OnRun()
    {
        if(!isRun()) false;

        TimeForMsgCount();

        fd_set fdRead;
        //fd_set fdWrite;
        //fd_set fdError;

        FD_ZERO(&fdRead);
        //FD_ZERO(&fdWrite);
        //FD_ZERO(&fdError);

        FD_SET(_listenSock, &fdRead);
        //FD_SET(_listenSock, &fdWrite);
        //FD_SET(_listenSock, &fdError);

        timeval tv { 0, 10000 }; //10ms
        int ret = select((int)_listenSock + 1, &fdRead, nullptr, nullptr, &tv);
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

        return true;
    }

    bool isRun()
    {
        return _listenSock != INVALID_SOCKET;
    }

    //响应消息包
    void TimeForMsgCount()
    {
        double timeinterval = _timeStamp.getElapsedSecond();
        if(timeinterval >= 1.0)
        {
            int recvCount = 0;
            for(auto* pCellServer : _cellServers)
            {
                recvCount += pCellServer->RecvCount;
                pCellServer->RecvCount = 0;
            }

            printf("thread<%d>,time=<%lf>,Sock=<%d>,clients<%d>,recvPackCount=<%d>\n",kCellServer_Thread_Count, timeinterval, (int)_listenSock, (int)_clients.size(), (int)(recvCount / timeinterval));
            _timeStamp.Update();
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

    virtual void OnLeave(ClientSocket* pClient) override
    {
        _clients.remove(pClient);
    }
private:
    SOCKET _listenSock = INVALID_SOCKET;
    std::list<ClientSocket*> _clients;
    std::vector<CellServer*> _cellServers;  

    TimeStamp _timeStamp;
};

#endif
