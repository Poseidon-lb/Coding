#pragma once
#define _GNU_SOURCE
#include "Channel.hpp"
#include "EventLoop.hpp"
#include <strings.h>
#include "Dispatcher.hpp"
#include <sys/epoll.h>
#include <string>

using namespace std;

class EpollDispatcher : public Dispatcher {
public:
    EpollDispatcher(EventLoop* evLoop);
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 监测事件
    int dispatch(int timeout = 2) override;  
    // 虚析构
    ~EpollDispatcher();
private:
    int epollCtl(int op);
private:
    int m_epfd;
    epoll_event* m_events;
    const int m_Max = 520;
};