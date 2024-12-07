#include "Channel.hpp"

Channel::Channel(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg) {
    m_fd = fd;
    m_events = events;
    readCallBack = readFunc;
    writeCallBack = writeFunc;
    m_arg = arg;
}

void Channel::writeEventEnable(bool flag) {
    if (flag) {
        m_events |= static_cast<int>(FdEvent::WriteEvent);
    } else {
        m_events &= ~static_cast<int>(FdEvent::WriteEvent);
    }
}

bool Channel::isWriteEventEntable() {
    return m_events & static_cast<int>(FdEvent::WriteEvent);   
}