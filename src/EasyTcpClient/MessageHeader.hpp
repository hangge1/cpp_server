#pragma once

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