#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "Channel.hpp"
#include "Dispatcher.hpp"
#include <sys/socket.h>
#include "TcpConnection.hpp"
#include <strings.h>
#include <mutex>
#include <thread>
#include <queue>
#include <map>
#include "Log.h"

//处理对channel的方式
enum class ElemType : char {
    ADD, DELETE, MODIFY
};

// 定义任务队列的节点
struct ChannelElement {
    ElemType type;                   // 添加/删除/修改 处理对应的channel
    Channel *channel;    // 被检测的描述符
};

class Dispatcher;
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
private:
    bool m_isQuit;
    Dispatcher* m_dispatcher;
    // 任务队列
    queue<ChannelElement*> m_taskQ; 
    // map
    map<int, Channel*> channelMap;
    // 线程id， name. mutex
    thread::id m_threadID; 
    char m_threadName[32];
    pthread_mutex_t m_mutex;
    int m_socketPair[2];      // 通过本地通信的fd, 通过socketpair初始化
};

// 初始化一个eventloop
struct EventLoop* eventLoopInit();
struct EventLoop* eventLoopInitEx(const char *threadName);
// 启动反应堆实例
void eventLoopRun(struct EventLoop *evLoop);
// 处理被激活的文件描述符
int eventActivate(struct EventLoop *evLoop, int fd, int event);
// 添加任务到任务队列
void eventLoopAddTask(struct EventLoop *evLoop, struct Channel *channel, int type);
// 处理任务队列中的任务
void eventLoopProcessTask(struct EventLoop* evLoop);
// 处理dsipatcher中的节点
int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel);
int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel);
int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel);
// 释放channel
int destroyChannel(struct EventLoop *evLoop, struct Channel *channel);