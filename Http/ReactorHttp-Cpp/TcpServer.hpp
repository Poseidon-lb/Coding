#pragma ocne
#define _GNU_SOURCE
#include "Log.h"
#include <strings.h>
#include "ThreadPool.h"

struct Listener {
    int lfd;
    unsigned short port;
};

struct TcpServer {
    int threadNum;
    struct EventLoop *mainLoop;
    struct ThreadPool *threadpool;
    struct Listener *listener;  
};

// 初始化监听的文件描述符
struct Listener* listenerInit(unsigned short port);
// 初始化
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
// 启动服务器
void tcpServerRun(struct TcpServer *server);