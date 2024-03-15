#include "Channel.h"
#include "EventLoop.h"
#include "doctest.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <sys/epoll.h>

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;
auto console = std::make_shared<spdlog::sinks::stdout_sink_mt>();

TEST_CASE("Set Callback and handle event") {
  ringbuffer_sink->set_pattern("%v");
  sinks.push_back(ringbuffer_sink);
  logger = std::make_shared<spdlog::logger>("logger_name", std::begin(sinks),
                                            std::end(sinks));
  EventLoop loop;
  Channel channel(&loop, 0);
  auto read_cb = []() { logger->info("readCallback"); };
  auto write_cb = []() { logger->info("writeCallback"); };
  auto error_cb = []() { logger->info("errorCallback"); };
  channel.setReadCallback(read_cb);
  channel.setWriteCallback(write_cb);
  channel.setErrorCallback(error_cb);

  SUBCASE("read callback") {
    channel.set_revents(EPOLLIN);
    channel.handleEvent();
    std::string expected{"readCallback\n"};
    std::string text = ringbuffer_sink->last_formatted().back();
    CHECK(expected == text);
  }
  SUBCASE("write callback") {
    channel.set_revents(EPOLLOUT);
    channel.handleEvent();
    std::string expected{"writeCallback\n"};
    std::string text = ringbuffer_sink->last_formatted().back();
    CHECK(expected == text);
  }
  SUBCASE("error callback") {
    channel.set_revents(EPOLLERR);
    channel.handleEvent();
    std::string expected{"errorCallback\n"};
    std::string text = ringbuffer_sink->last_formatted().back();
    CHECK(expected == text);
  }
}
