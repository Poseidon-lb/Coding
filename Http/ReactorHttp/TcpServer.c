#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"

struct Listener* listenerInit(unsigned short port) {
    struct Listener *listener = (struct Listener *)malloc(sizeof (struct Listener));
    listener->port = port;
    // 创建监听的套接字
    listener->lfd = socket(AF_INET, SOCK_STREAM, 0);
    // 设置端口复用, 2MSL(TCP协议)的等待时长,大概一分钟
    int opt = 1;
    setsockopt(listener->lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    // 绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener->lfd, (struct sockaddr *)&addr, sizeof addr);
    // 设置监听
    listen(listener->lfd, 128);
    return listener;    
}

struct TcpServer* tcpServerInit(unsigned short port, int threadNum) {
    struct TcpServer* tcp = (struct TcpServer *)malloc(sizeof (struct TcpServer));
    tcp->listener = listenerInit(port);
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadpool = threadPoolInit(tcp->mainLoop, threadNum);
    Debug("服务器初始化完成");
    return tcp;
}

void* acceptConnection(void *arg) {
    Debug("主线程的读回调");
    struct TcpServer *server = (struct TcpServer *)arg;
    // 和客户端建立连接
    int cfd = accept(server->listener->lfd, NULL, NULL);
    Debug("连接成功");
    struct EventLoop *evLoop = takeWorkerEventLoop(server->threadpool);
    Debug("找到一个子线程的evLoop");
    // 将cfd放到TcpConnection中去处理
    tcpConnectionInit(cfd, evLoop);
    return NULL;
}

void tcpServerRun(struct TcpServer *server) {
    // 启动线程池
    threadPoolRun(server->threadpool);
    Debug("线程池启动完成");
    // 添加检测反应堆
    struct Channer *channel = ChannelInit(server->listener->lfd, 
        ReadEvent, acceptConnection, NULL, server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    Debug("监听的文件描述符已经加入主线程的mainLoop");
    
    // 启动反应堆模型
    eventLoopRun(server->mainLoop);
}