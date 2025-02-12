// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

struct ErrorResult : DataHeader
{
    ErrorResult()
    {
        dataLength = sizeof(ErrorResult);
        cmd = CMD_ERROR;
    }
};

int main()
{
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

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
        std::cout << "error: " << WSAGetLastError() << "\n";
        return -1;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);
    servAddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");

    if(SOCKET_ERROR == bind(listenSock, (sockaddr*)&servAddr, sizeof(servAddr)))
    {
        std::cout << "错误, 绑定网络端口失败...\n";
        std::cout << "error: " << WSAGetLastError() << "\n";
        return -1;
    }

    if(SOCKET_ERROR == listen(listenSock, 5))
    {
        std::cout << "错误, 监听网络端口失败...\n";
        std::cout << "error: " << WSAGetLastError() << "\n";
        return -2;
    }

    sockaddr_in clientAddr{};
    int addrLen = sizeof(sockaddr_in);
    SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
    if(INVALID_SOCKET == clientSock)
    {
        std::cout << "错误, Accept客户端失败...\n";
        std::cout << "error: " << WSAGetLastError() << "\n";
        return -3;
    }
    std::cout << "New Client: IP = " << inet_ntoa(clientAddr.sin_addr) << "\n";

    while(true)
    {
        DataHeader header {};

        int nRecv = recv(clientSock, (char*)&header, sizeof(DataHeader), 0);
        if(nRecv <= 0)
        {
            std::cout << "客户端断开连接!\n";
            break;
        }

        printf("收到命令: %d 数据长度: %d\n", header.cmd, header.dataLength);
        
        switch( header.cmd )
        {
        case CMD_LOGIN:
            {
                Login login;
                recv(clientSock, (char*)&login + sizeof(header), sizeof(login) - sizeof(header), 0);
                printf("收到Login命令,长度=%d, username: %s, password: %s\n", login.dataLength, login.userName, login.passWord);
                //暂时忽略验证过程
                LoginResult loginRes;
                send(clientSock, (char*)&loginRes, sizeof(loginRes), 0);
            }
            break;
        case CMD_LOGOUT:
            {
                Logout logout;
                recv(clientSock, (char*)&logout + sizeof(header), sizeof(logout) - sizeof(header), 0);
                printf("收到Logout命令,长度=%d, username: %s\n", logout.dataLength, logout.userName);
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
    }
    
    closesocket(listenSock);

    std::cout << "Server Exit\n";
    WSACleanup();

    getchar();

    return 0;
}
