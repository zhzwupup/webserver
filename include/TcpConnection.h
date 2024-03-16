#pragma once

#include "Buffer.h"
#include "Callback.h"
#include "InetAddress.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

extern std::shared_ptr<spdlog::logger> logger;

class Channel;
class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }

  bool connected() const { return state_ == StateE::kConnected; }

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
  std::string name_;
  std::atomic<StateE> state_;

  bool reading_;

  std::unique_ptr<Channel> channel_;
  int sockfd_;

  InetAddress localAddr_;
  InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  CloseCallback closeCallback_;

  Buffer inputBuffer_;
  Buffer outputBuffer_;
};
