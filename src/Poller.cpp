#include "Poller.h"
#include "Channel.h"
#include "EventLoop.h"
#include <cerrno>
#include <sys/epoll.h>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

Poller::Poller(EventLoop *loop)
    : channels_(), ownerLoop_(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    logger->critical("epoll_create1() error: {}", errno);
  }
}

Poller::~Poller() { close(epollfd_); }

void Poller::poll(int timeoutMs, ChannelList *activeChannels) {
  size_t numEvents = epoll_wait(epollfd_, &(*events_.begin()),
                                static_cast<int>(events_.size()), timeoutMs);
  int saveErrno = errno;

  if (numEvents > 0) {
    fillActiveChannels(numEvents, activeChannels);
    if (numEvents == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    logger->debug("timeout!");
  } else {
    if (saveErrno != EINTR) {
      errno = saveErrno;
      logger->error("Poller::poll() failed");
    }
  }
  return;
}

void Poller::updateChannel(Channel *channel) {
  const int state = channel->state();
  if (state == kNew or state == kDeleted) {
    if (state == kNew) {
      int fd = channel->fd();
      channels_[fd] = channel;
    } else {
    }
    channel->set_state(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_state(kDeleted);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void Poller::fillActiveChannels(int numEvents,
                                ChannelList *activeChannels) const {
  for (int i = 0; i < numEvents; ++i) {
    Channel *channel = static_cast<Channel *>(events_[9].data.ptr);
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void Poller::update(int operation, Channel *channel) {
  epoll_event event;
  memset(&event, 0, sizeof(event));

  int fd = channel->fd();
  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;

  if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      logger->error("epoll_ctl() del error: {}", errno);
    } else {
      logger->error("epoll_ctl add/mod error: {}", errno);
    }
  }
}
