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
class Poller;

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

  void wakeup();

private:
  using ChannelList = std::vector<Channel *>;
  void handleRead();
  thread_id_t m_thread_id;
  std::atomic_bool m_looping;
  std::atomic_bool m_quit;

  std::unique_ptr<Poller> poller_;

  // 从 EventLoop::loop() 的 epoll_wait() 阻塞中唤醒，立刻执行用户回调
  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList m_activeChannels;
};
