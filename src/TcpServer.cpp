#include "TcpServer.h"
#include "Acceptor.h"
#include "Callback.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"

#include "spdlog/spdlog.h"
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>

extern std::shared_ptr<spdlog::logger> logger;

static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    logger->critical("mainLoop is null!");
  }
  return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg, Option option)
    : loop_(CheckLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()),
      name_(nameArg), acceptor_(std::make_unique<Acceptor>(
                          loop, listenAddr, option == Option::kReusePort)),
      threadPool_(std::make_shared<EventLoopThreadPool>(loop, name_)),
      connectionCallback_(), messageCallback_(), writeCompleteCallback_(),
      threadInitCallback_(), started_(0), nextConnId_(1) {
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

TcpServer::~TcpServer() {
  for (auto &item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads) {
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
  if (started_++ == 0) {
    threadPool_->start(threadInitCallback_);
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  EventLoop *ioLoop = threadPool_->getNextLoop();

  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);

  ++nextConnId_;

  std::string connName = name_ + buf;
  // logger->info("TcpServer::newConnection [{}] - new connection [{}] from {}",
  //              name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

  sockaddr_in local;
  memset(&local, 0, sizeof(local));
  socklen_t addrlen = sizeof(local);
  if (getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0) {
    logger->error("get local addr failed");
  }

  InetAddress localAddr(local);
  TcpConnectionPtr conn = std::make_shared<TcpConnection>(
      ioLoop, connName, sockfd, localAddr, peerAddr);
  connections_[connName] = conn;

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);

  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
  // logger->info("TcpServer::removeConnectionInLoop [{}] - connection [{}]",
  //              name_.c_str(), conn->name().c_str());

  connections_.erase(conn->name());
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
