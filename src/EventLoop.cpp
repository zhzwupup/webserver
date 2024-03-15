#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <sstream>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>

thread_local EventLoop *t_loopInThisThread = nullptr;

constexpr int kPollTimeMs = 10000000;

int createEventfd() {
  int evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evfd < 0) {
    logger->critical("eventfd error: {}", errno);
  }
  return evfd;
}

EventLoop::EventLoop()
    : m_thread_id(std::this_thread::get_id()), m_looping(false), m_quit(false),
      poller_(std::make_unique<Poller>(this)), wakeupFd_(createEventfd()),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {
  if (t_loopInThisThread) {
    std::stringstream msg_buf;
    msg_buf << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << m_thread_id;
    logger->critical(msg_buf.str());
  } else {
    t_loopInThisThread = this;
  }

  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  assert(!m_looping);
  t_loopInThisThread = nullptr;
}

void EventLoop::assertInLoopThread() {
  if (!isInLoopThread()) {
    // abort();
    logger->critical("Run in error thread, should abort");
  }
}
void EventLoop::loop() {
  assert(!m_looping);
  assertInLoopThread();
  m_looping = true;
  if (isInLoopThread()) {
    std::stringstream msg_buf;
    msg_buf << "EventLoop run in thread " << m_thread_id;
    logger->info(msg_buf.str());
    poller_->poll(kPollTimeMs, &m_activeChannels);
    for (Channel *channel : m_activeChannels) {
      channel->handleEvent();
    }
  }
  m_looping = false;
}

void EventLoop::quit() {
  m_quit = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb);
  }

  if (!isInLoopThread() or callingPendingFunctors_) {
    wakeup();
  }
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    logger->error("EventLoop::wakeup writes {} bytes instead of 8", n);
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof(one));
  logger->debug("EventLoop::handleRead()");
  if (n != sizeof(one)) {
    logger->error("EventLoop::handleRead() reads {} bytes instead of 8", n);
  }
}

void EventLoop::updateChannel(Channel *channel) {
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  poller_->removeChannel(channel);
}
