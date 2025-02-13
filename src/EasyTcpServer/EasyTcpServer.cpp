// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>

#ifdef _WIN32
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


enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RES,
    CMD_LOGOUT,
    CMD_LOGOUT_RES, 
    CMD_NEW_USER_JOIN, 
    CMD_ERROR
};

//包头
struct DataHeader
{
    short dataLength;
    short cmd;
};

//包体
struct Login : DataHeader
{   
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32] {0};
    char passWord[32] {0};
};

struct LoginResult : DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RES;
        result = 0;
    }
    int result;
};

struct Logout : DataHeader
{
    Logout()
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName[32] {0};
};

struct LogoutResult : DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RES;
        result = 0;
    }
    int result;
};

struct NewUserJoin : DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        newUserSocket = 0;
    }
    int newUserSocket;
};

struct ErrorResult : DataHeader
{
    ErrorResult()
    {
        dataLength = sizeof(ErrorResult);
        cmd = CMD_ERROR;
    }
};

int Processor(SOCKET clientSock)
{
    char buf[256]{0};

    int nRecv = (int)recv(clientSock, (char*)buf, sizeof(DataHeader), 0);  
    if(nRecv <= 0)
    {
        printf("客户端[%d]断开连接!\n", (int)clientSock);
        return -1;
    }
    printf("recv ret = %d\n", nRecv);

    DataHeader* header = (DataHeader*)buf;
    printf("客户端[%d] 发送命令: %d 数据长度: %d\n",(int)clientSock, header->cmd, header->dataLength);
        
    switch( header->cmd )
    {
    case CMD_LOGIN:
        {
            Login* login = (Login*)header;
            recv(clientSock, (char*)login + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到Login命令,长度=%hd, username: %s, password: %s\n", login->dataLength, login->userName, login->passWord);
            //暂时忽略验证过程
            LoginResult loginRes;
            send(clientSock, (char*)&loginRes, sizeof(loginRes), 0);
        }
        break;
    case CMD_LOGOUT:
        {
            Logout* logout = (Logout*)header;
            recv(clientSock, (char*)logout + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到Logout命令,长度=%d, username: %s\n", logout->dataLength, logout->userName);
            //暂时忽略验证过程
            LogoutResult logoutRes;
            send(clientSock, (char*)&logoutRes, sizeof(logoutRes), 0);
        }
        break;
    default:
        printf("receive unknown cmd!\n");
        ErrorResult errorRes;
        send(clientSock, (char*)&errorRes, sizeof(errorRes), 0);
        break;
    }

    return 0;
}

int main()
{
#ifdef _WIN32
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);
#endif

    //--简易TCP服务器, 步骤如下: 
    // 1 创建socket
    // 2 bind
    // 3 listen
    // 4 accept
    // 5 send
    // 6 recv
    // 7 close

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == listenSock)
    {
        std::cout << "错误, SOCKET创建失败...\n";
        return -1;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);
    servAddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");

    if(SOCKET_ERROR == bind(listenSock, (sockaddr*)&servAddr, sizeof(servAddr)))
    {
        std::cout << "错误, 绑定网络端口失败...\n";
        return -1;
    }

    if(SOCKET_ERROR == listen(listenSock, 5))
    {
        std::cout << "错误, 监听网络端口失败...\n";
        return -2;
    }

    std::vector<SOCKET> clients;

    fd_set fdRead;
    fd_set fdWrite;
    fd_set fdError;
    while(true)
    {
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdError);

        FD_SET(listenSock, &fdRead);
        FD_SET(listenSock, &fdWrite);
        FD_SET(listenSock, &fdError);
        SOCKET maxSock = listenSock;

        for(int n = (int)clients.size() - 1; n >= 0; n--)
        {
            FD_SET(clients[n], &fdRead);
            maxSock = std::max<int>((int)maxSock, (int)clients[n]);
        }

        timeval tv { 2, 0 };
        int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdError, &tv);
        if(ret < 0)
        {
            printf("select任务结束!\n");
            break;
        }

        //检测客户端连接
        if(FD_ISSET(listenSock, &fdRead))
        {
            FD_CLR(listenSock, &fdRead);

            sockaddr_in clientAddr{};
            int addrLen = sizeof(sockaddr_in);

#ifdef _WIN32
            SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
#else
            SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, (socklen_t*)&addrLen);
#endif
            if(INVALID_SOCKET == clientSock)
            {
                printf("错误, Accept客户端失败...\n");
            }
            else
            {
                //群发消息通知玩家登录
                for(int n = (int)clients.size()-1; n >=0; n--)
                {
                    NewUserJoin msg;
                    msg.newUserSocket = (int)clientSock;
                    send(clients[n], (char*)&msg, sizeof(msg), 0);
                }

                printf("新客户端登录Socket=%d Ip=%s\n", (int)clientSock, inet_ntoa(clientAddr.sin_addr));
                clients.emplace_back(clientSock);
            }     
        }

        auto it = clients.begin();
        while(it != clients.end())
        {
            if(FD_ISSET(*it, &fdRead))
            {
                if(-1 == Processor(*it))
                {
                    it = clients.erase(it);
                    continue;
                }
            }
            ++it;
        }  

        printf("空间时间处理其他业务...\n");
    }
    
#ifdef _WIN32
    //关闭所有套接字
    for(int n = (int)clients.size() - 1; n >= 0; n--)
    {
        closesocket(clients[n]);
    }
    closesocket(listenSock);
    WSACleanup();
#else
    for(int n = (int)clients.size() - 1; n >= 0; n--)
    {
        close(clients[n]);
    }
    close(listenSock);
#endif

    std::cout << "Server Exit\n";

    getchar();

    return 0;
}
