#include "Channel.h"
#include "EventLoop.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : m_loop(loop), m_fd(fd), m_events(0), m_revents(0), m_index(-1) {}

void Channel::update() { m_loop->updateChannel(this); }
