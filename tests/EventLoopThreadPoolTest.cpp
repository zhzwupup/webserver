#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "Poller.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <chrono>
#include <iostream>
#include <spdlog/common.h>
#include <thread>

using namespace std::chrono_literals;

void EventLoop::quit() {}
EventLoop::~EventLoop() {}
EventLoop::EventLoop() {}
void EventLoop::loop() {
  logger->debug("EventLoop::loop");
  // 在这里短暂睡眠，防止 EventLoopThread::threadFunc 退出后，loop_ 被设为
  // nullptr, 而 EventLoopThread::startLoop 被唤醒时看到 loop_ 是 nullptr,
  // 之后永远不能被唤醒
  std::this_thread::sleep_for(500ms);
  return;
}

Poller::~Poller() {}

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
std::shared_ptr<spdlog::logger> logger =
    std::make_shared<spdlog::logger>("console", console_sink);

int main() {
  console_sink->set_level(spdlog::level::trace);
  console_sink->set_pattern("[%^%l%$] %v");
  logger->set_level(spdlog::level::debug);
  EventLoop baseLoop;
  EventLoopThreadPool pool(&baseLoop, "pool");
  pool.setThreadNum(10);
  // pool.start([](EventLoop *) {
  //   std::cout << "running in thread pool" << std::endl;
  //   return;
  // });
  pool.start();
}
