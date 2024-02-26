#include "Channel.h"
#include "doctest.h"
#include "EventLoop.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <sys/epoll.h>

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;
auto console = std::make_shared<spdlog::sinks::stdout_sink_mt>();

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : m_loop(loop), m_fd(fd), m_events(0), m_revents(0), m_index(-1) {}

void Channel::update() { logger->info("m_loop->updateChannel(this)"); }

TEST_CASE("Enable Reading") {
  ringbuffer_sink->set_pattern("%v");
  sinks.push_back(ringbuffer_sink);
  logger = std::make_shared<spdlog::logger>("logger_name", std::begin(sinks),
                                            std::end(sinks));
  EventLoop loop;
  Channel channel(&loop, 0);
  channel.enableReading();

  std::string expected{"m_loop->updateChannel(this)\n"};
  std::string text = ringbuffer_sink->last_formatted().back();
  CHECK(expected == text);
}
