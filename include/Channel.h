#pragma once
#include <functional>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/ringbuffer_sink.h>

extern std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink;
extern std::vector<spdlog::sink_ptr> sinks;
extern std::shared_ptr<spdlog::logger> logger;
class EventLoop;

class Channel {
public:
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;

  using EventCallback = std::function<void()>;
  Channel(EventLoop *loop, int fd);

  void handleEvent();
  void setReadCallback(const EventCallback &cb) { m_readCallback = cb; }
  void setWriteCallback(const EventCallback &cb) { m_writeCallback = cb; }
  void setErrorCallback(const EventCallback &cb) { m_errorCallback = cb; }

  int fd() const { return m_fd; }
  int events() const { return m_events; }
  void set_revents(int revt) { m_revents = revt; }
  bool isNoneEvent() const { return m_events == kNoneEvent; }

  void enableReading() {
    m_events |= kReadEvent;
    update();
  }
  void enableWriting() {
    m_events |= kWriteEvent;
    update();
  }
  void disableWriting() {
    m_events &= ~kWriteEvent;
    update();
  }
  void disableAll() {
    m_events = kNoneEvent;
    update();
  }

  int state() { return m_state; }
  void set_state(int state) { m_state = state; }

  EventLoop *ownerLoop() { return m_loop; }

private:
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *m_loop;
  const int m_fd;
  int m_events;
  int m_revents;
  int m_state;

  EventCallback m_readCallback;
  EventCallback m_writeCallback;
  EventCallback m_closeCallback;
  EventCallback m_errorCallback;
};
