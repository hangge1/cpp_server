// EasyTcpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

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
        return -1;
    }
   
    switch( header->cmd )
    {
    case CMD_LOGIN_RES:
        {
            LoginResult* login_res = (LoginResult*)header;
            recv(clientSock, (char*)login_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_LOGIN_RES, 数据长度: %d\n", login_res->dataLength);
        }
        break;
    case CMD_LOGOUT_RES:
        {
            LogoutResult* logout_res = (LogoutResult*)header;
            recv(clientSock, (char*)logout_res + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_LOGOUT_RES, 数据长度: %d\n", logout_res->dataLength);
        }
        break;
    case CMD_NEW_USER_JOIN:
        {
            NewUserJoin* newJoin = (NewUserJoin*)header;
            recv(clientSock, (char*)newJoin + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            printf("收到服务器消息: CMD_NEW_USER_JOIN, 数据长度: %d\n", newJoin->dataLength);
        }
        break;
    }

    return 0;
}

int main()
{
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

    //--简易TCP客户端, 步骤如下: 
    // 1 创建socket
    // 2 connect
    // 3 send
    // 4 recv
    // 5 close

    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == clientSock)
    {
        std::cout << "错误, SOCKET创建失败...\n";
        return -1;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);
    servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    if(SOCKET_ERROR == connect(clientSock, (sockaddr*)&servAddr, sizeof(servAddr)))
    {
        std::cout << "错误, Connect失败...\n";
        return -2;
    }

    char cmdBuf[128] {0};
    while(true)
    {
        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(clientSock, &fdReads);

        timeval tv {2, 0};
        int ret = select((int)clientSock, &fdReads, nullptr, nullptr, &tv);
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
        Login login;
        strncpy_s(login.userName, "zzh", 32);
        strncpy_s(login.passWord, "123456", 32);
        int nSend = send(clientSock, (char*)&login, sizeof(Login), 0);
        printf("send ret = %d\n", nSend);
        printf("空间时间处理其他业务...\n");
        Sleep(2000);

        //std::cin >> cmdBuf;
        //if(!strncmp(cmdBuf, "exit", 128))
        //{
        //    break;
        //}

        //if(!strncmp(cmdBuf, "login", 128))
        //{
        //    //发
        //    Login login;
        //    strncpy_s(login.userName, "zzh", 32);
        //    strncpy_s(login.passWord, "123456", 32);
        //    send(clientSock, (char*)&login, sizeof(login), 0);

        //    //收
        //    LoginResult loginResponse;
        //    recv(clientSock, (char*)&loginResponse, sizeof(loginResponse), 0);
        //    printf("LoginResult: %d\n", loginResponse.result);
        //}
        //else if(!strncmp(cmdBuf, "logout", 128))
        //{
        //    //发
        //    Logout logout;
        //    strncpy_s(logout.userName, "zzh", 32);
        //    send(clientSock, (char*)&logout, sizeof(logout), 0);

        //    //收
        //    LogoutResult logoutResponse;
        //    recv(clientSock, (char*)&logoutResponse, sizeof(logoutResponse), 0);
        //    printf("LogoutResult: %d\n", logoutResponse.result);
        //}
        //else
        //{
        //    printf("命令不支持!\n");
        //}
    }
    
    closesocket(clientSock);
    WSACleanup();

    getchar();
    return 0;
}
