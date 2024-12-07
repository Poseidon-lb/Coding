#include "Dispatcher.h"
#include <stdio.h>
#include <sys/epoll.h>

#define Max 520

struct EpollData {
    int epfd;
    struct epoll_event* events;
};

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollDispatch(struct EventLoop* evLoop, int timeout);
static int epollClear(struct EventLoop* evLoop);

struct Dispatcher EpollDispatcher = {
    epollInit, 
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear
};

static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op) {
    struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd;
    int events = 0;
    if (channel->events & ReadEvent) {
        events = EPOLLIN;
    }
    if (channel->events & WriteEvent) {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret;
}

static void* epollInit() {
    struct EpollData* data = (struct EpollData*)malloc(sizeof (struct EpollData));
    data->epfd = epoll_create(1);
    if (data->epfd == -1) {
        perror("epoll_create");
        exit(0);
    }
    data->events = (struct epoll_event*)calloc(Max, sizeof (struct epoll_event));
    return data;
}

static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) {
    return epollCtl(channel, evLoop, EPOLL_CTL_ADD);
}

static int epollRemove(struct Channel* channel, struct EventLoop* evLoop) {
    return epollCtl(channel, evLoop, EPOLL_CTL_DEL);
}

static int epollModify(struct Channel* channel, struct EventLoop* evLoop) {
    return epollCtl(channel, evLoop, EPOLL_CTL_MOD);
}

static int epollDispatch(struct EventLoop* evLoop, int timeout) {
    Debug("epoll模型正在检测");
    struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
    int count = epoll_wait(data->epfd, data->events, Max, timeout * 1000);
    for (int i = 0; i < count; i ++) {
        int fd = data->events[i].data.fd;
        int events = data->events[i].events;
        if ((events & EPOLLERR) || (events & EPOLLHUP)) {
            // 对方已经断开连接 or 对方已经断开连接还在发数据
            //epollRemove(evLoop->)
            continue;
        } 
        if (events & EPOLLIN) {
            Debug("有客户端连接");
            eventActivate(evLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT) {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }
    return 0;
}

static int epollClear(struct EventLoop* evLoop) {
    struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}
