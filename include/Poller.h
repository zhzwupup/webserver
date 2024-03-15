#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>

class Poller {
public:
  using ChannelList = std::vector<Channel *>;
  using EventList = std::vector<epoll_event>;

  Poller(EventLoop *loop);
  ~Poller();
  void poll(int timeoutMs, ChannelList *activaChannels);
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);

  bool hasChannel(Channel *channel) const;

private:
  static constexpr int kInitEventListSize = 16;

  void fillActiveChannels(int numEvents, ChannelList *activaChannels) const;
  void update(int operation, Channel *channel);

  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap channels_;
  EventLoop *ownerLoop_;

  int epollfd_;
  EventList events_;
};
