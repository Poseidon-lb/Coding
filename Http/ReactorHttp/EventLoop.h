#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <sys/socket.h>
#include "TcpConnection.h"
#include <strings.h>
#include "Log.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher pollDispatcher;

//处理对channel的方式
enum  ElemType {
    ADD, DELETE, MODIFY
};

// 定义任务队列的节点
struct ChannelElement {
    int type;                   // 添加/删除/修改 处理对应的channel
    struct Channel *channel;    // 被检测的描述符
    struct ChannelElement *next;
};

struct Dispatcher;

struct EventLoop {
    bool isQuit;
    struct Dispatcher* dispatcher;
    void* dispatcherData;
    // 任务队列
    struct ChannelElement *head, *tail;
    // map
    struct ChannelMap *map;
    // 线程id， name. mutex
    pthread_t threadID; 
    char threadName[32];
    pthread_mutex_t mutex;
    int socketPair[2];      // 通过本地通信的fd, 通过socketpair初始化
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