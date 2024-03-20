#include "EventLoopThread.h"
#include "EventLoop.h"
#include <condition_variable>
#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string_view &name)
    : loop_(nullptr), exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(),
      cond_(), callback_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != nullptr) {
    loop_->quit();
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  thread_.start();

  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // logger->debug("EventLoopThread::startLoop start cond wait");
    cond_.wait(lock, [this]() { return loop_ != nullptr; });
    // logger->debug("EventLoopThread::startLoop end cond wait");
    loop = loop_;
  }
  return loop;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.loop();

  std::unique_lock<std::mutex> lock(mutex_);
  loop_ = nullptr;
}
