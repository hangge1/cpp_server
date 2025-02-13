**应用层调用send和recv究竟发生了什么?**

内核区存在SendBuffer和RecvBuffer; 

send把待发送数据放到内核的SendBuffer中, recv从内核缓冲区RecvBuffer读数据!



**那么内核缓冲区的SendBuffer何时真正通过网卡发出呢?**

由网络协议栈决定, 因此发送的数据并不一定等于你应用层送到内核缓冲区的数据大小! 

可能仅仅只是你发送的数据包一部分,  或者是你的数据包一部分和别的包合并的数据!等等



**那么如何保证TCP通信的数据正确解析呢?**

TCP保证数据不会丢和有序! 所以应用层的任务就是确保接收更多的数据包, 根据数据包的长度, 按顺序解析数据包



**那么如何能够尽可能的提高通信效率呢?**

应用层维护额外的读写缓冲区,  每次recv的时候尽可能大的读取更多的数据, 放到应用层缓冲区, 如果够包头大小就解析获取长度, 如果够长度, 就解析包体; 不够就继续读!



**什么是粘包呢? 半包呢?  如何解决呢? **

复杂网络环境中, 接收到的数据>一个包长度, 导致一次接收接收到不同数据包的数据

半包就是接收到的数据不足一个包长度!

**核心处理逻辑:**

```c++
#define RECV_BUFF_SIZE 10240
//接收缓冲区
char _recvBuf[RECV_BUFF_SIZE] {};
//消息缓冲区(第二缓冲区)
char _msgBuf[RECV_BUFF_SIZE*10] {};
int _lastPos = 0;

//接收数据[处理粘包 拆分包]
int ReceiveData(SOCKET clientSock)
{
    int nRecv = recv(clientSock, (char*)_recvBuf, RECV_BUFF_SIZE, 0);
    if(nRecv <= 0)
    {
        return -1;
    }

    memcpy(_msgBuf + _lastPos, _recvBuf, nRecv);
    _lastPos += nRecv;
    while (_lastPos > sizeof(DataHeader)) //如果够多个包循环处理, 解决粘包
    {
        DataHeader* header = (DataHeader*)_msgBuf;
        int packageSize = header->dataLength;
        if(_lastPos > packageSize) //够一个包长度
        {
            OnNetMsg(header); //处理包
            memmove(_msgBuf, _msgBuf + packageSize, _lastPos - packageSize);
            _lastPos -= packageSize;
        }
        else break; //不够一个包长度, 就退出, 解决半包问题
    }
    return 0;
}
```





