#pragma once

#include "Thread.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <string_view>

extern std::shared_ptr<spdlog::logger> logger;

class EventLoop;

class EventLoopThread {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string_view &name = std::string());
  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};
