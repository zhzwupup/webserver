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
  }
  m_looping = false;
}
