#include "TcpConnection.h"
#include "Buffer.h"
#include "Callback.h"
#include "Channel.h"
#include "EventLoop.h"
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

void setKeepAlive(int sockfd, bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval,
             sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void shudtownWrite(int sockfd) {
  if (shutdown(sockfd, SHUT_WR) < 0) {
    logger->error("shudtownWrite error");
  }
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(loop), name_(nameArg), state_(StateE::kConnecting), reading_(true),
      sockfd_(sockfd), channel_(std::make_unique<Channel>(loop, sockfd)),
      localAddr_(localAddr), peerAddr_(peerAddr) {
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  // SPDLOG_LOGGER_INFO(logger, "TcpConnection::ctor[{}] at fd = {}",
  //                    name_.c_str(), sockfd);

  setKeepAlive(sockfd_, true);
}

TcpConnection::~TcpConnection() {
  close(sockfd_);
  // SPDLOG_LOGGER_INFO(logger, "TcpConnection::dtor[{}] at fd = {} state = {}",
  //                    name_.c_str(), channel_->fd(),
  //                    static_cast<int>(state_));
}

void TcpConnection::send(const std::string &buf) {
  if (state_ == StateE::kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf.c_str(), buf.size());
    } else {
      void (TcpConnection::*fp)(const void *data, size_t len) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp, this, buf.c_str(), buf.size()));
    }
  }
}

void TcpConnection::send(Buffer *buf) {
  if (state_ == StateE::kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      void (TcpConnection::*fp)(const std::string &message) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
    }
  }
}

void TcpConnection::sendInLoop(const std::string &message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;

  if (state_ == StateE::kDisconnected) {
    logger->error("disconnected, give up writing");
    return;
  }

  // channel 第一次写数据，且缓冲区没有待发送数据
  if (!channel_->isWriting() and outputBuffer_.readableBytes() == 0) {
    nwrote = write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        logger->error("TcpConnection::sendInLoop");
        if (errno == EPIPE or errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  // 没有一次性发送完数据，剩余数据保存到缓冲区
  if (!faultError and remaining > 0) {
    outputBuffer_.append((char *)data + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown() {
  if (state_ == StateE::kConnected) {
    setState(StateE::kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  if (!channel_->isWriting()) {
    shudtownWrite(sockfd_);
  }
}

void TcpConnection::connectEstablished() {
  setState(StateE::kConnected);

  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  // SPDLOG_LOGGER_INFO(logger, "connectDestroyed");
  if (state_ == StateE::kConnected) {
    setState(StateE::kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead() {
  int savedErrno = 0;

  ssize_t n = inputBuffer_.ReadFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    logger->error("TcpConnection::handleRead() failed");
    handleError();
  }
}

void TcpConnection::handleWrite() {
  if (channel_->isWriting()) {
    int savedErrno = 0;
    ssize_t n = outputBuffer_.WriteFd(channel_->fd(), &savedErrno);

    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == StateE::kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      logger->error("TcpConnection::handleWrite() failed");
    }
  } else {
    logger->error("TcpConnection fd={} is down, no more writing",
                  channel_->fd());
  }
}

void TcpConnection::handleClose() {
  setState(StateE::kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr);
  ;
}

void TcpConnection::handleError() {
  int optval;
  socklen_t optlen = sizeof(optval);
  int err = 0;

  if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
    err = errno;
  } else {
    err = optval;
  }

  logger->error("TcpConnection::handleError name:  - SO_ERROR: {}", err);
}
