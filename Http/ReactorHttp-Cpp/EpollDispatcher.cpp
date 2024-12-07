#include "EpollDispatcher.hpp"
#include <stdio.h>
#include <sys/epoll.h>

int EpollDispatcher::epollCtl(int op) {
    struct epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    int events = 0;
    if (m_channel->events & (int)FdEvent::ReadEvent) {
        events = EPOLLIN;
    }
    if (m_channel->events & (int)FdEvent::WriteEvent) {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
}

EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop) {
    m_epfd = epoll_create(1);
    if (m_epfd == -1) {
        perror("epoll_create");
        exit(0);
    }
    m_events = new epoll_event[m_Max];
}

int EpollDispatcher::add() {
    return epollCtl(EPOLL_CTL_ADD);
}

int EpollDispatcher::remove() {
    return epollCtl(EPOLL_CTL_DEL);
}

int EpollDispatcher::modify() {
    return epollCtl(EPOLL_CTL_MOD);
}

int EpollDispatcher::dispatch(int timeout) {
    Debug("epoll模型正在检测");
    int count = epoll_wait(m_epfd, m_events, m_Max, timeout * 1000);
    for (int i = 0; i < count; i ++) {
        int fd = m_events[i].data.fd;
        int events = m_events[i].events;
        if ((events & EPOLLERR) || (events & EPOLLHUP)) {
            // 对方已经断开连接 or 对方已经断开连接还在发数据
            //epollRemove(evLoop->)
            continue;
        } 
        if (events & EPOLLIN) {
            Debug("有客户端连接");
            eventActivate(m_evLoop, fd, FdEvent::ReadEvent);
        }
        if (events & EPOLLOUT) {
            eventActivate(m_evLoop, fd, FdEvent::WriteEvent);
        }
    }
    return 0;
}

EpollDispatcher::~EpollDispatcher() {
    close(m_epfd);
    delete []m_events;
    return 0;
}
