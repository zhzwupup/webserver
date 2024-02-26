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

extern thread_local EventLoop *t_loopInThisThread;

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;

TEST_CASE("EventLoop Constructor") {
  ringbuffer_sink->set_pattern("%v");
  sinks.push_back(ringbuffer_sink);
  logger = std::make_shared<spdlog::logger>("logger_name", std::begin(sinks),
                                            std::end(sinks));
  sinks.push_back(ringbuffer_sink);

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
