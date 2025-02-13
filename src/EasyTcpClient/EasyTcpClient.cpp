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
