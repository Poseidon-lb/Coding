#include "WorkerThread.h"
#include "Log.h"

int workerThreadInit(struct WorkerThread *thread, int index) {
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name, "SonThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}

// 子线程的回调函数
void* SonTreadRunning(void *arg) {
    struct WorkerThread *thread = (struct WorkerThread *)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    eventLoopRun(thread->evLoop);
    return NULL; 
}

void wokerThreadRun(struct WorkerThread *thread) {
    // 创建子线程, 将thread传递到子线程
    pthread_create(&thread->threadID, NULL, SonTreadRunning, thread);
    // 阻塞主线程，等子线程evloop初始化完成
    pthread_mutex_lock(&thread->mutex);
    while (thread->evLoop == NULL) {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}
