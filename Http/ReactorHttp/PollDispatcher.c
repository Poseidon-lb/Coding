#include "Dispatcher.h"
#include <stdio.h>
#include <poll.h>

#define Max 1024

struct PollData {
    int mxfd;
    struct pollfd fds[Max];
};

static void* pollInit();
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int pollModify(struct Channel* channel, struct EventLoop* evLoop);
static int pollDispatch(struct EventLoop* evLoop, int timeout);
static int pollClear(struct EventLoop* evLoop);

struct Dispatcher pollDispatcher = {
    pollInit, 
    pollAdd,
    pollRemove,
    pollModify,
    pollDispatch,
    pollClear
};

static void* pollInit() {
    struct PollData* data = (struct PollData*)malloc(sizeof (struct PollData));
    data->mxfd = 0;
    for (int i = 0; i < Max; i ++) {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
}

int max(int x, int y) {
    if (x < y) {
        return y;
    }
    return x;
}

static int pollAdd(struct Channel* channel, struct EventLoop* evLoop) {
    struct PollData* data = (struct PollData*)evLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent) {
        events = POLLIN;
    }
    if (channel->events & WriteEvent) {
        events |= POLLOUT;
    }
    int ok = 0;
    for (int i = 0; i < Max; i ++) {
        if (data->fds[i].fd == -1) {
            ok = 1;
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->mxfd = max(data->mxfd, i);
            break;
        }
    }
    if (ok == 0) {
        return -1;
    }
    return 0;
}

static int pollRemove(struct Channel* channel, struct EventLoop* evLoop) {
    struct PollData* data = (struct PollData*)evLoop->dispatcherData;
    for (int i = 0; i < data->mxfd; i ++) {
        if (data->fds[i].fd == channel->fd) {
            data->fds[i].fd = -1;
            data->fds[i].events = data->fds[i].revents = 0;
            break;
        }
    }
    return 0;
}

static int pollModify(struct Channel* channel, struct EventLoop* evLoop) {
    struct PollData* data = (struct PollData*)evLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent) {
        events = POLLIN;
    }
    if (channel->events & WriteEvent) {
        events |= POLLOUT;
    }
    for (int i = 0; i < data->mxfd; i ++) {
        if (data->fds[i].fd == channel->fd) {
            data->fds[i].events = events;
            break;
        }
    }
    return 0;
}

static int pollDispatch(struct EventLoop* evLoop, int timeout) {
    struct PollData* data = (struct PollData*)evLoop->dispatcherData;
    int count = poll(data->fds, data->mxfd + 1, timeout * 1000);
    for (int i = 0; i < data->mxfd; i ++) {
        int revent = data->fds[i].revents;
        if (revent & POLLIN) {
            eventActivate(evLoop, data->fds[i].fd, ReadEvent);

        }
        if (revent & POLLOUT) {
            eventActivate(evLoop, data->fds[i].fd, WriteEvent);
        }
    }
    return 0;
}

static int pollClear(struct EventLoop* evLoop) {
    struct pollData* data = (struct pollData*)evLoop->dispatcherData;
    free(data);
    return 0;
}
