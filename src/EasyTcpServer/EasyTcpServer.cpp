// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

struct DataPackage
{
    int age;
    char name[32];
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

    char msgBuf[128] {};
    while(true)
    {
        int nRecv = recv(clientSock, msgBuf, 128, 0);
        if(nRecv <= 0)
        {
            std::cout << "客户端断开连接!\n";
            break;
        }

        std::cout << "recv: " << msgBuf << std::endl;
        
        if(!strncmp(msgBuf, "getInfo", 128))
        {
            DataPackage pack { 25, "Zhang zhihang" };
            send(clientSock, (char*)&pack, sizeof(pack), 0);
        }
        else
        {
            const char* unknownBuf = "unknown cmd!";
            send(clientSock, unknownBuf, strlen(unknownBuf) + 1, 0);
        }
    }
    
    closesocket(listenSock);

    std::cout << "Server Exit\n";
    WSACleanup();

    getchar();

    return 0;
}
