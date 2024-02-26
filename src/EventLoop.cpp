#include "EventLoop.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <sstream>
#include <thread>

thread_local EventLoop *t_loopInThisThread = nullptr;

EventLoop::EventLoop()
    : m_looping(false), m_thread_id(std::this_thread::get_id()) {
  if (t_loopInThisThread) {
    std::stringstream msg_buf;
    msg_buf << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << m_thread_id;
    logger->critical(msg_buf.str());
  } else {
    t_loopInThisThread = this;
  }
}

EventLoop::~EventLoop() {
  assert(!m_looping);
  t_loopInThisThread = nullptr;
}
