#ifndef _MessageHeader_H_
#define _MessageHeader_H_

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
    DataHeader()
    {
        dataLength = sizeof(DataHeader);
        cmd = CMD_ERROR;
    }
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
    char test[32] {0};
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
    char test[92] {0};
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

#endif