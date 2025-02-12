// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
                Login login {};
                recv(clientSock, (char*)&login, sizeof(login), 0);
                printf("recv user login, username: %s, password: %s\n", login.userName, login.passWord);
                //暂时忽略验证过程
                LoginResult loginRes { 0 };
                send(clientSock, (char*)&header, sizeof(header), 0);
                send(clientSock, (char*)&loginRes, sizeof(loginRes), 0);
            }
            break;
        case CMD_LOGOUT:
            {
                Logout logout {};
                recv(clientSock, (char*)&logout, sizeof(logout), 0);
                printf("recv user logout, username: %s\n", logout.userName);
                //暂时忽略验证过程
                LogoutResult logoutRes { 0 };
                send(clientSock, (char*)&header, sizeof(header), 0);
                send(clientSock, (char*)&logoutRes, sizeof(logoutRes), 0);
            }
            break;
        default:
            printf("receive unknown cmd!\n");
            DataHeader resHeader { 0, CMD_ERROR };
            send(clientSock, (char*)&resHeader, sizeof(resHeader), 0);
            break;
        }
    }
    
    closesocket(listenSock);

    std::cout << "Server Exit\n";
    WSACleanup();

    getchar();

    return 0;
}
