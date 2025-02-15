// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include "EasyTcpServer.hpp"

bool g_bRun = true;
void CmdFunc(EasyTcpServer* server)
{
    if(!server) return;

    char cmdBuf[128] {0};
    while(server->isRun())
    {
        scanf("%s", cmdBuf);

        if(!strncmp(cmdBuf, "exit", 128))
        {
            printf("退出cmd线程!\n");
            server->Close();
            break;
        }
        else
        {
            printf("命令不支持!\n");
        }
    }
}

class MyServer : public EasyTcpServer
{
public:
    //客户端加入事件
    virtual void OnNetJoin(ClientSocket* pClient) override
    {
        EasyTcpServer::OnNetJoin(pClient);

        //printf("client join, sock=<%d>\n", (int)pClient->sockfd());
    }

    //客户端断开事件
    virtual void OnNetLeave(ClientSocket* pClient) override
    {
        EasyTcpServer::OnNetLeave(pClient);

        printf("client leave, sock=<%d>\n", (int)pClient->sockfd());
    }

    //客户端消息事件
    virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) override
    {
        EasyTcpServer::OnNetMsg(pClient, header);

        //printf("recv client cmd %d, sock=<%d>\n", header->cmd ,(int)pClient->sockfd());
        switch( header->cmd )
        {
        case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                //printf("收到客户端<%d>请求: CMD_LOGIN, 数据长度=%hd, username: %s\n",(int)clientSock, login->dataLength, login->userName);
                //LoginResult loginRes;
                //pClient->SendData(&loginRes);
            }
            break;
        case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                //printf("收到客户端<%d>请求: CMD_LOGOUT, 数据长度=%d, username: %s\n",(int)clientSock, logout->dataLength, logout->userName);
                //LogoutResult logoutRes;
                //pClient->SendData(&logoutRes);
            }
            break;
        default:
            {
                printf("收到客户端<%d>未知的请求! 数据长度=%d\n", (int)pClient->sockfd(), header->dataLength);
                ErrorResult errorRes;
                pClient->SendData(&errorRes);
            }       
            break;
        }
    }
private:
    
};


int main()
{
    MyServer server;
    //EasyTcpServer server;
    if(server.Bind(nullptr, 9090) < 0)
    {
        printf("Bind Error!\n");
        return -1;
    }

    if(server.Listen(5) < 0)
    {
        printf("Listen Error!\n");
        return -2;
    }
    
    server.Start(4);

    std::thread cmdThread(CmdFunc, &server);
    cmdThread.detach();

    while(server.isRun())
    {
        server.OnRun();
    }
    
    std::cout << "Server Exit\n";

    server.Close();
    getchar();

    return 0;
}
