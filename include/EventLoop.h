#pragma once
#include <atomic>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>
#include <thread>

using thread_id_t = std::thread::id;
extern std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink;
extern std::vector<spdlog::sink_ptr> sinks;
extern std::shared_ptr<spdlog::logger> logger;

class Channel;

class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  void loop();

  bool isInLoopThread() const {
    return m_thread_id == std::this_thread::get_id();
  }

  void assertInLoopThread();

  void updateChannel(Channel *channel);

private:
  thread_id_t m_thread_id;
  std::atomic_bool m_looping;
};
