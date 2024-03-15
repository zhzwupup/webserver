#pragma once

#include "Channel.h"
#include "EventLoop.h"

class EventLoop;
class InetAddress;

class Acceptor {
public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &)>;
  Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  bool listenning() const { return listenning_; }
  void listen();

private:
  void handleRead();

  EventLoop *loop_;
  int acceptSock_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
};
