// EasyTcpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

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

    char recvBuf[256] {};
    int nRecv = recv(clientSock, recvBuf, 256, 0);
    if(nRecv > 0)
    {
        std::cout << "接收到数据: " << recvBuf << "\n";
    }
    closesocket(clientSock);
    WSACleanup();

    getchar();
    return 0;
}
