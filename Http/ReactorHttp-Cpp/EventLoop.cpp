#include "EventLoop.h"
#include <strings.h>

struct EventLoop* eventLoopInitEx(const char *threadName) {
    struct EventLoop *evLoop = (struct EventLoop *)malloc(sizeof (struct EventLoop));
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL);
    strcpy(evLoop->threadName, threadName);
    evLoop->dispatcher = &EpollDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();
    evLoop->head = evLoop->tail = NULL;
    evLoop->map = ChannelMapInit(128);
    socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    // 指定规则: evLoop->socketPair[0] 发送数据
    struct Channel *channel = ChannelInit(evLoop->socketPair[1], ReadEvent, NULL, NULL, NULL);
    eventLoopAddTask(evLoop, channel, ADD);
    eventLoopProcessTask(evLoop);
    return evLoop;
}

struct EventLoop* eventLoopInit() {
    return eventLoopInitEx("MainThread");
}

int eventActivate(struct EventLoop* evLoop, int fd, int event) {
    if (fd < 0 || evLoop == NULL) {
        return -1;
    }
    struct Channel* channel = evLoop->map->list[fd];
    if (event & ReadEvent && channel->readCallBack != NULL) {
        Debug("触发读回调");
        channel->readCallBack(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallBack != NULL) {
        channel->writeCallBack(channel->arg);
    }
    return 0;
}

int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel) {
    int fd = channel->fd;
    struct ChannelMap *map = evLoop->map;
    if (!makeMapRoom(map, fd, sizeof (struct Channel*))) {
        return -1;
    }
    if (map->list[fd] == NULL) {
        map->list[fd] = channel;
        evLoop->dispatcher->add(channel, evLoop);
    }
    return 0;
}

int destroyChannel(struct EventLoop *evLoop, struct Channel *channel) {
    if (evLoop != NULL || channel != NULL) {
        return -1;
    }
    int fd = channel->fd;
    struct ChannelMap *map = evLoop->map;
    map->list[fd] = NULL;
    close(fd);
    free(channel);
    return 0;
}

int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel) {
    int fd = channel->fd;
    struct ChannelMap *map = evLoop->map;
    if (fd >= map->size) {
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}

int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel) {
    int fd = channel->fd;
    struct ChannelMap *map = evLoop->map;
    if (map->list[fd] == NULL) {
        return -1;
    }
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}

void eventLoopProcessTask(struct EventLoop* evLoop) {
    pthread_mutex_lock(&evLoop->mutex);
    while (evLoop->head != NULL) {
        int type = evLoop->head->type;
        struct Channel *channel = evLoop->head->channel;
        if (type == ADD) {
            Debug("监听的文件描述符已经加入反应堆");
            eventLoopAdd(evLoop, channel);
        }
        if (type == DELETE) {
            eventLoopRemove(evLoop, channel);
            tcpConnectionDestroy((struct TcpConnection *)channel->arg);
        }
        if (type == MODIFY) {
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement *t = evLoop->head;
        evLoop->head = evLoop->head->next;
        free(t);
    }
    evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
}

void taskWakeup(struct EventLoop *evLoop) {
    const char *msg = "hello";
    write(evLoop->socketPair[0], msg, strlen(msg));
}

void eventLoopAddTask(struct EventLoop *evLoop, struct Channel *channel, int type) {
    // 加锁,保护共享资源
    pthread_mutex_lock(&evLoop->mutex);
    struct ChannelElement *node = (struct ChannelElement *)malloc(sizeof (struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    if (evLoop->head == NULL) {
        evLoop->head = evLoop->tail = node;
    } else {
        evLoop->tail = evLoop->tail->next = node;
    }
    pthread_mutex_unlock(&evLoop->mutex);
    /*
        处理节点
        主线程只负责给子线程添加任务，不能处理
    */
    if (evLoop->threadID == pthread_self()) {
        // 子线程
        eventLoopProcessTask(evLoop);
    } else {
        // 主线程 - 告诉子线程处理任务, 得先打断阻塞
        taskWakeup(evLoop);
    }
}

void eventLoopRun(struct EventLoop* evLoop) {
    assert(evLoop != NULL);
    if (evLoop->threadID != pthread_self()) {
        return;
    }
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    while (!evLoop->isQuit) {
        dispatcher->dispatch(evLoop, 2);    //超时时长2s
        eventLoopProcessTask(evLoop);
    }
}