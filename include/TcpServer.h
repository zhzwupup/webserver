#pragma once

#include "Acceptor.h"
#include "Callback.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include <functional>

class TcpServer {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  enum class Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &nameArg, Option option = Option::kNoReusePort);
  ~TcpServer();

  void setThreadInitCallback(const ThreadInitCallback &cb) {
    threadInitCallback_ = cb;
  }
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

  void setThreadNum(int numThreads);

  void start();

  EventLoop *getLoop() const { return loop_; }

  const std::string name() { return name_; }

  const std::string ipPort() { return ipPort_; }

private:
  void newConnection(int sockfd, const InetAddress &peerAddr);
  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

  EventLoop *loop_; // baseLoop

  std::string ipPort_;
  std::string name_;
  std::unique_ptr<Acceptor> acceptor_;

  std::shared_ptr<EventLoopThreadPool> threadPool_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;

  ThreadInitCallback threadInitCallback_;
  std::atomic_int started_;

  int nextConnId_;
  ConnectionMap connections_;
};
