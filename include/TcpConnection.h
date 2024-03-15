#pragma once

#include "Buffer.h"
#include "Callback.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

extern std::shared_ptr<spdlog::logger> logger;

class Channel;
class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, int sockfd);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }

  void send(const std::string &buf);
  void send(Buffer *buf);

  void shutdown();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  void connectEstablished();
  void connectDestroyed();

private:
  enum class StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void setState(StateE state) { state_ = state; }
  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const void *message, size_t len);
  void sendInLoop(const std::string &message);
  void shutdownInLoop();

  EventLoop *loop_;
  std::atomic<StateE> state_;

  bool reading_;

  std::unique_ptr<Channel> channel_;
  int sockfd_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  CloseCallback closeCallback_;

  Buffer inputBuffer_;
  Buffer outputBuffer_;
};
