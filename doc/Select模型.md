**最基本1对1的阻塞式CS的程序结构**

<img src="Select%E6%A8%A1%E5%9E%8B/image-20250213013517128.png" alt="image-20250213013517128" style="zoom:50%;" />

**致命缺点:**  服务器无法处理多个客户端连接





**服务器配合Select模型处理多客户端**

![image-20250213013718523](Select%E6%A8%A1%E5%9E%8B/image-20250213013718523.png)

```c++
//最核心函数
int WSAAPI
select(
    _In_ int nfds, 
    //Windows下没有意义, linux和MacOS下有意义
    //linux和MacOS下设置为当前最大描述符 + 1
    //Windows下Select最大默认支持64
    _Inout_opt_ fd_set FAR * readfds,
    _Inout_opt_ fd_set FAR * writefds,
    _Inout_opt_ fd_set FAR * exceptfds,
    _In_opt_ const struct timeval FAR * timeout
    );

typedef struct fd_set {
        u_int fd_count;
        SOCKET  fd_array[FD_SETSIZE];
} fd_set;

//宏函数操作fd_set
//1 FD_ZERO(set)
//2 FD_SET(fd, set)
//3 FD_ISSET(fd, set)
//4 FD_CLR(fd, set)
```

**思考几个问题:** 

**Select怎样支持多客户端的, 基本的通信逻辑描述一下?** 

答: 

1 select调用前, 将所有客户端和服务器的SOCKET设置到fd_set中

2 select调用后, 遍历select的活跃SOCKET数组

如果是服务器就代表有客户端接入, 如果是客户端代表客户端发消息需要服务器处理!



**select函数的第一个参数, 最后一个参数, 返回值代表什么意思?** 

1 Windows下没有意义, Linux和MacOS下表示最大描述符+1

2 最后一个参数是等待时长(nullptr表示阻塞, 0表示非阻塞, >0指定最大阻塞时间)

3 返回值代表活跃的SOCKET数量



**Select内部维护fd集合使用的数据结构是怎样的?  Select的优点和缺点?** 

静态数组, 数组长度通过宏定义写死!  

优点: 跨平台, 几乎所有平台都支持; 简单易用

缺点: 每次都需要重复添加关注的SOCKET



**客户端使用Select模型改造**

模拟聊天室场景:  新客户端连接服务器, 通知所有其他已连接客户端

因为接入了Select, 原本阻塞等待用户输入命令的功能, 必须引入多线程



Select模型在Windows下最大支持64, Linux最大支持1024个

**突破Windows Select的64限制方法:** 

```
#define FD_SETSIZE 1024
#include <WinSock2.h>

Window内部是这样的
#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif /* FD_SETSIZE */
```



**Select的优化:** 

利用Visual Stdio自带的性能分析器, 我们得到性能热点主要集中在: 

**1 select函数(没办法直接优化)**

只能后面使用IOCP进行优化网络模型

**2 每一次循环开始前, 需要将所有客户端的sock加入fd_set(可以优化)** [**FD_SET宏的操作很耗时**]

```c++
SOCKET maxSock = (*_clients.begin())->sockfd();
for(auto it = _clients.begin(); it != _clients.end(); ++it)
{
    FD_SET((*it)->sockfd(), &fdRead); //性能热点
    maxSock = std::max<int>((int)maxSock, (int)(*it)->sockfd());
}
```

**优化思路:**  只要没有客户端加入和离开, 该fd_set不需要变化! 直接拷贝一份就行!

```
1 成员添加bool变量, 表示是否客户端数量有变化

2 成员添加fd_set_bak, 表示上次备份的fd_set集合

3 每次onRun的时候, 检测bool变化, 如果有变化就重新计算, 拷贝给fd_set_bak, 设置标志为false; 否则直接用fd_set_bak

4 新客户端连接和有客户端退出的时候设置bool为true, 表示有变化
```



**3 针对所有的客户端检测IS_SET的时候(可以优化)**

在linux, macOS是没有办法优化的, 但是Windows可以优化因为windows的fd_set带有fd_count这个成员, 但是我们可以遍历活跃的sock, 如果是已存在的客户端SOCKET, 就可以处理!

需要手段:  快速检测SOCKET是否是客户端, 所以需要引入map



**4 解决粘包的时候, 移动缓冲区数据特别频繁(可以优化)**

如果每一次解析一个完整的包都要移动缓冲区, 这样效率就特别低,  我们需要尽可能少的移动!





