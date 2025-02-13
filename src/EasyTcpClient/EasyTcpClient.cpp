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
void CmdFunc()
{
    //if(!client) return;

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
            //client->SendData(&login);
        }
        else if(!strncmp(cmdBuf, "logout", 128))
        {
            //发
            Logout logout;
            strncpy(logout.userName, "zzh", 32);
            //client->SendData(&logout);
        }
        else
        {
            printf("命令不支持!\n");
        }
    }  
}

int main()
{
    const int clientCount = FD_SETSIZE - 1;
    EasyTcpClient* client[clientCount] {};
    for(int i = 0; i < clientCount; i++)
    {
        client[i] = new EasyTcpClient();
        if(-1 == client[i]->Connect("127.0.0.1", 9090))
        {
            printf("Client Connect failed!\n");
            return -1;
        }
    }


    //类似UI线程
    std::thread cmdThread(CmdFunc);
    cmdThread.detach();
    
    Login login;
    strncpy(login.userName, "zzh", 32);
    strncpy(login.passWord, "123456", 32);
    while(g_bRun)
    {
        for(int i = 0; i < clientCount; i++)
        {
            client[i]->SendData(&login);
            client[i]->OnRun();
        }
    }

    for(int i = 0; i < clientCount; i++)
    {
        client[i]->Close();
        client[i] = nullptr;
    }

    getchar();
    return 0;
}
