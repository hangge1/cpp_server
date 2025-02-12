// EasyTcpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

enum CMD
{
    CMD_LOGIN,
    CMD_LOGOUT,
    CMD_ERROR
};

//包头
struct DataHeader
{
    short dataLength;
    short cmd;
};

//包体
struct Login
{
    char userName[32];
    char passWord[32];
};

struct LoginResult
{
    int result;
};

struct Logout
{
    char userName[32];
};

struct LogoutResult
{
    int result;
};

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

    char cmdBuf[128] {};
    while(true)
    {
        std::cin >> cmdBuf;
        if(!strncmp(cmdBuf, "exit", 128))
        {
            break;
        }

        if(!strncmp(cmdBuf, "login", 128))
        {
            //发
            Login login { "zzh", "123456" };
            DataHeader header { sizeof(Login), CMD_LOGIN };
            send(clientSock, (char*)&header, sizeof(header), 0);
            send(clientSock, (char*)&login, sizeof(login), 0);

            //收
            DataHeader responseHeader {};
            LoginResult loginResponse {};
            recv(clientSock, (char*)&responseHeader, sizeof(responseHeader), 0);
            recv(clientSock, (char*)&loginResponse, sizeof(loginResponse), 0);

            printf("LoginResult: %d\n", loginResponse.result);
        }
        else if(!strncmp(cmdBuf, "logout", 128))
        {
            //发
            Logout logout { "zzh" };
            DataHeader header { sizeof(Logout), CMD_LOGOUT };
            send(clientSock, (char*)&header, sizeof(header), 0);
            send(clientSock, (char*)&logout, sizeof(logout), 0);

            //收
            DataHeader responseHeader {};
            LogoutResult logoutResponse {};
            recv(clientSock, (char*)&responseHeader, sizeof(responseHeader), 0);
            recv(clientSock, (char*)&logoutResponse, sizeof(logoutResponse), 0);

            printf("LogoutResult: %d\n", logoutResponse.result);
        }
        else
        {
            printf("命令不支持!\n");
        }
    }
    
    closesocket(clientSock);
    WSACleanup();

    getchar();
    return 0;
}
