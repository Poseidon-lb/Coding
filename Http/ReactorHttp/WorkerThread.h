#pragma once 
#include "EventLoop.h"
#include <stdio.h>

// 定义子线程结构体
struct WorkerThread {
    pthread_t threadID;
    char name[24];
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond;   // 条件变量
    struct EventLoop *evLoop;
};

// 初始化
int workerThreadInit(struct WorkerThread *thread, int index);
// 启动线程
void wokerThreadRun(struct WorkerThread *thread);