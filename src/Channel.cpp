#include "Channel.h"
#include "EventLoop.h"

#include <spdlog/spdlog.h>
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : m_loop(loop), m_fd(fd), m_events(0), m_revents(0), m_state(-1),
      tied_(false) {}

Channel::~Channel() {
  // SPDLOG_LOGGER_INFO(logger, "Channel::dtor");
}

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() { m_loop->updateChannel(this); }

void Channel::remove() { m_loop->removeChannel(this); }

void Channel::handleEvent() {
  if (tied_) {
    std::shared_ptr<void> guard = tie_.lock();
    if (guard) {
      handleEventWithGuard();
    }
  } else {
    handleEventWithGuard();
  }
}

void Channel::handleEventWithGuard() {
  if ((m_revents & EPOLLHUP) and not(m_revents & EPOLLIN)) {
    if (m_closeCallback) {
      m_closeCallback();
    }
  }

  if (m_revents & (EPOLLERR)) {
    logger->error("the fd = {}", this->fd());
    if (m_errorCallback) {
      m_errorCallback();
    }
  }

  if (m_revents & (EPOLLIN | EPOLLPRI)) {
    // logger->debug("channel have read events, the fd = {}", this->fd());
    if (m_readCallback) {
      m_readCallback();
    }
  }

  if (m_revents & EPOLLOUT) {
    if (m_writeCallback) {
      m_writeCallback();
    }
  }
}
