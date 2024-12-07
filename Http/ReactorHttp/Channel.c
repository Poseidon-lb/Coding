#include "Channel.h"

struct Channel* ChannelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg) {
    struct Channel* channel = (struct Channel*)malloc(sizeof (struct Channel));
    channel->fd = fd;
    channel->events = events;
    channel->readCallBack = readFunc;
    channel->writeCallBack = writeFunc;
    channel->arg = arg;
    return channel;
}

void writeEventEnable(struct Channel* channel, bool flag) {
    if (flag) {
        channel->events |= WriteEvent;
    } else {
        channel->events &= ~WriteEvent;
    }
}

bool isWriteEventEntable(struct Channel * channel) {
    return channel->events & WriteEvent;   
}