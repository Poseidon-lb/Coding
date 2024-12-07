#include "ThreadPool.h"

struct ThreadPool* threadPoolInit(struct EventLoop *mainLoop, int count) {
    struct ThreadPool* pool = (struct ThreadPool *) malloc(sizeof (struct ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->threadNum = count;
    pool->workerThreads = (struct WorkerThread *)malloc(count * sizeof (struct WorkerThread));
    return pool;
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool *pool) {
    assert(pool->isStart);
    if (pool->mainLoop->threadID != pthread_self()) {
        exit(0);
    }
    // 从线程池中找一个子线程，然后取出反应堆实例
    struct EventLoop *evLoop = pool->mainLoop;
    if (pool->threadNum > 0) {
        evLoop = pool->workerThreads[pool->index++].evLoop;
        pool->index %= pool->threadNum;
    }
    return evLoop;
}

void threadPoolRun(struct ThreadPool *pool) {
    assert(pool && !pool->isStart);
    if (pool->mainLoop->threadID != pthread_self()) {
        exit(0);
    }
    Debug("准备启动线程池");
    pool->isStart = true;
    if (pool->threadNum > 0) {
        for (int i = 0; i < pool->threadNum; i ++) {
            workerThreadInit(pool->workerThreads + i, i + 1);
            wokerThreadRun(pool->workerThreads + i);
        }
    }
}