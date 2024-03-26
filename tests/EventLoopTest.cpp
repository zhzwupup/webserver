#include "EventLoop.h"
#include "doctest.h"
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>

#include "spdlog/sinks/stdout_sinks.h"

extern thread_local EventLoop *t_loopInThisThread;

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;
auto console = std::make_shared<spdlog::sinks::stdout_sink_mt>();

TEST_CASE("EventLoop Constructor") {
  ringbuffer_sink->set_pattern("%v");
  sinks.push_back(ringbuffer_sink);
  // sinks.push_back(console);
  logger = std::make_shared<spdlog::logger>("logger_name", std::begin(sinks),
                                            std::end(sinks));

  SUBCASE("Normally constructed") {
    t_loopInThisThread = nullptr;
    EventLoop eventLoop;
    CHECK(&eventLoop == t_loopInThisThread);
  }

  SUBCASE("Another EventLoop exists in this thread") {
    t_loopInThisThread = (EventLoop *)0x12;
    EventLoop eventLoop;

    std::string text = ringbuffer_sink->last_formatted().back();
    std::stringstream buf;
    // logger 会在字符串后加上一个换行符
    buf << "Another EventLoop " << t_loopInThisThread
        << " exists in this thread " << std::this_thread::get_id() << "\n";
    std::string expected = buf.str();
    CHECK(text == expected);
  }
}

TEST_CASE("EventLoop loop and quit") {
  using namespace std::chrono_literals;
  EventLoop loop;
  auto quit = std::thread([&loop]() {
    std::this_thread::sleep_for(1s);
    loop.quit();
  });
  loop.loop();
  quit.join();
}

TEST_CASE("runInLoop") {
  EventLoop loop;
  std::thread::id tid{};

  // 在 EventLoop 所属线程中调用 runInLoop
  loop.runInLoop([&tid]() { tid = std::this_thread::get_id(); });
  CHECK(tid == std::this_thread::get_id());

  // 在其他线程中调用 runInLoop
  auto thrd = std::thread([&loop, &tid]() {
    loop.runInLoop([&tid]() { tid = std::this_thread::get_id(); });
  });
  thrd.join();
  CHECK(tid == std::this_thread::get_id());
}

TEST_CASE("loop, waked up by runInLoop") {
  using namespace std::chrono_literals;

  EventLoop loop;
  auto run = std::thread([&loop]() {
    std::this_thread::sleep_for(3s);
    loop.runInLoop([&loop]() { loop.quit(); });
  });

  loop.loop();
  run.join();
}

// TEST_CASE("Create EventLoop and Run loop") {
//   ringbuffer_sink->set_pattern("%v");
//   sinks.push_back(ringbuffer_sink);
//   logger = std::make_shared<spdlog::logger>("logger_name", std::begin(sinks),
//                                             std::end(sinks));
//
//   SUBCASE("Create and Run EventLoop in same thread") {
//     EventLoop loop;
//     loop.loop();
//
//     std::string text = ringbuffer_sink->last_formatted().back();
//     std::stringstream buf;
//     buf << "EventLoop run in thread " << std::this_thread::get_id() << "\n";
//     std::string expected = buf.str();
//     CHECK(text == expected);
//
//     auto threadFunc = []() {
//       EventLoop loop;
//       loop.loop();
//     };
//     std::thread thrd(threadFunc);
//     buf.clear();
//     buf.str("");
//     buf << "EventLoop run in thread " << thrd.get_id() << "\n";
//     expected = buf.str();
//     thrd.join();
//     text = ringbuffer_sink->last_formatted().back();
//     CHECK(text == expected);
//   }
//
//   SUBCASE("Run EventLoop in another thread") {
//     EventLoop loop;
//     auto threadFunc = [&loop]() { loop.loop(); };
//     std::thread thrd(threadFunc);
//     std::string expected{"Run in error thread, should abort\n"};
//     thrd.join();
//
//     std::string text = ringbuffer_sink->last_formatted().back();
//     CHECK(text == expected);
//   }
// }
