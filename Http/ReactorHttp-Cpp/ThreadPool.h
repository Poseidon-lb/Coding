#pragma once
#define _GNU_SOURCE
#include "Log.h"
#include "WorkerThread.h"
#include <stdlib.h>

// 定义线程池
struct ThreadPool {
    struct EventLoop *mainLoop;
    bool isStart;
    int threadNum;
    struct WorkerThread *workerThreads;
    int index;
}; 

// 初始化线程池
struct ThreadPool* threadPoolInit(struct EventLoop *mainLoop, int count);
// 启动线程池
void threadPoolRun(struct ThreadPool *pool);
// 取出某个子线程反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool *pool);