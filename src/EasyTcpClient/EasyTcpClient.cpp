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

#include "EasyTcpClient.hpp"


bool g_bRun = true;
void CmdFunc(EasyTcpClient* client)
{
    if(!client) return;

    char cmdBuf[128] {0};
    while(client->isRun())
    {
        scanf("%s", cmdBuf);

        if(!strncmp(cmdBuf, "exit", 128))
        {
            printf("退出cmd线程!\n");
            break;
        }

        if(!strncmp(cmdBuf, "login", 128))
        {
            //发
            Login login;
            strncpy(login.userName, "zzh", 32);
            strncpy(login.passWord, "123456", 32);
            client->SendData(&login);
        }
        else if(!strncmp(cmdBuf, "logout", 128))
        {
            //发
            Logout logout;
            strncpy(logout.userName, "zzh", 32);
            client->SendData(&logout);
        }
        else
        {
            printf("命令不支持!\n");
        }
    }  
    
    client->Close();
}

int main()
{
    EasyTcpClient client;
    if(-1 == client.Connect("127.0.0.1", 9090))
    {
        return -1;
    }

    //类似UI线程
    std::thread cmdThread(CmdFunc, &client);
    cmdThread.detach();
    
    while(client.isRun())
    {
        client.OnRun();
    }

    getchar();
    return 0;
}
