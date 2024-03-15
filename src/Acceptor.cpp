#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking() {
  int sockfd =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    logger->critical("listen socket create err {}", errno);
  }
  return sockfd;
}

static void setReuseAddr(int sockfd, bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

static void setReusePort(int sockfd, bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

static void bindAddress(int sockfd, const InetAddress &localaddr) {
  if (0 !=
      bind(sockfd, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in))) {
    logger->critical("bind sockfd: {} fail", sockfd);
  }
}

static void listenSock(int sockfd) {
  if (0 != listen(sockfd, 1024)) {
    logger->critical("listen sockfd: {} faile", sockfd);
  }
}

static int acceptSock(int sockfd, InetAddress *peeraddr) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));

  int connfd =
      accept4(sockfd, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

  if (connfd >= 0) {
    peeraddr->setSockAddr(addr);
  } else {
    logger->critical("accept4() failed");
  }
  return connfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop), acceptSock_(createNonblocking()),
      acceptChannel_(loop, acceptSock_), listenning_(false) {
  logger->debug("Acceptor create nonblocking socket, [fd = {}]",
                acceptChannel_.fd());

  setReuseAddr(acceptSock_, reuseport);
  setReusePort(acceptSock_, true);
  bindAddress(acceptSock_, listenAddr);

  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen() {
  listenning_ = true;
  listenSock(acceptSock_);
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  InetAddress peerAddr;

  int connfd = acceptSock(acceptSock_, &peerAddr);

  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      logger->debug("no newConnectionCallback_() function");
      close(connfd);
    }
  } else {
    logger->error("acceptSock() failed");
    if (errno == EMFILE) {
      logger->error("sockfd reached limit");
    }
  }
}
