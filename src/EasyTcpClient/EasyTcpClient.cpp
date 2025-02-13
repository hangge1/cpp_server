// EasyTcpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>

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
    char userName[32] {};
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

    int nRecv = recv(clientSock, (char*)buf, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)buf;
    if(nRecv <= 0)
    {
        printf("与服务器断开连接!\n");
        fflush(stdout);
        return -1;
    }
   
    switch( header->cmd )
    {
    case CMD_LOGIN_RES:
        {
            LoginResult* login_res = (LoginResult*)header;
            recv(clientSock, (char*)login_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_LOGIN_RES, 数据长度: %d\n", login_res->dataLength);
            fflush(stdout);
        }
        break;
    case CMD_LOGOUT_RES:
        {
            LogoutResult* logout_res = (LogoutResult*)header;
            recv(clientSock, (char*)logout_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_LOGOUT_RES, 数据长度: %d\n", logout_res->dataLength);
            fflush(stdout);
        }
        break;
    case CMD_NEW_USER_JOIN:
        {
            NewUserJoin* newJoin = (NewUserJoin*)header;
            recv(clientSock, (char*)newJoin + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_NEW_USER_JOIN, 数据长度: %d\n", newJoin->dataLength);
            fflush(stdout);
        }
        break;
    }

    fflush(stdout);
    return 0;
}


bool g_bRun = true;
void CmdFunc(SOCKET clientSock)
{
    char cmdBuf[128] {0};

    while(true)
    {
        scanf("%s", cmdBuf);

        if(!strncmp(cmdBuf, "exit", 128))
        {
            printf("退出cmd线程!\n");
            g_bRun = false;
            break;
        }

        if(!strncmp(cmdBuf, "login", 128))
        {
            //发
            Login login;
            strncpy(login.userName, "zzh", 32);
            strncpy(login.passWord, "123456", 32);
            send(clientSock, (char*)&login, sizeof(login), 0);
        }
        else if(!strncmp(cmdBuf, "logout", 128))
        {
            //发
            Logout logout;
            strncpy(logout.userName, "zzh", 32);
            send(clientSock, (char*)&logout, sizeof(logout), 0);
        }
        else
        {
            printf("命令不支持!\n");
        }
    }   
}

int main()
{
#ifdef _WIN32
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);
#endif

    //--简易TCP客户端, 步骤如下: 
    // 1 创建socket
    // 2 connect
    // 3 send
    // 4 recv
    // 5 close

    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == clientSock)
    {
        printf("错误, SOCKET创建失败...\n");
        return -1;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);

#ifdef _WIN32
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //servAddr.sin_addr.s_addr = inet_addr("192.168.26.129");
#else
    servAddr.sin_addr.s_addr = inet_addr("192.168.26.129");
#endif

    if(SOCKET_ERROR == connect(clientSock, (sockaddr*)&servAddr, sizeof(servAddr)))
    {
        printf("错误, Connect失败...\n");
        return -2;
    }

    std::thread cmdThread(CmdFunc, clientSock);
    cmdThread.detach();
    
    while(g_bRun)
    {
        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(clientSock, &fdReads);

        timeval tv {2, 0};
        int ret = select((int)clientSock + 1, &fdReads, nullptr, nullptr, &tv);
        if(ret < 0)
        {
            printf("select任务结束\n");
            break;
        }

        if(FD_ISSET(clientSock, &fdReads))
        {
            FD_CLR(clientSock, &fdReads);

            if(-1 == Processor(clientSock))
            {
                printf("2select任务结束\n");
                break;
            } 
        }


        //test
        //printf("空间时间处理其他业务...\n");    
    }

#ifdef _WIN32
    closesocket(clientSock);
    WSACleanup();
#else
    close(clientSock);
#endif

    getchar();
    return 0;
}
