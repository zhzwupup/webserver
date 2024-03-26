#include "Channel.h"
#include "EventLoop.h"
#include "Poller.h"
#include "doctest.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;
auto console = std::make_shared<spdlog::sinks::stdout_sink_mt>();

// Fake EventLoop
EventLoop::EventLoop()
    : m_thread_id(std::this_thread::get_id()), m_looping(false), m_quit(false),
      poller_(std::make_unique<Poller>(this)),
      wakeupFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {}
EventLoop::~EventLoop() {}

void EventLoop::updateChannel(Channel *channel) {}
void EventLoop::removeChannel(Channel *channel) {}
// Fake EventLoop end

//  Fake Poller
Poller::Poller(EventLoop *loop)
    : channels_(), ownerLoop_(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    logger->critical("epoll_create1() error: {}", errno);
  }
}
Poller::~Poller() { close(epollfd_); }

void Poller::poll(int timeoutMs, ChannelList *activaChannels) {}
void Poller::updateChannel(Channel *channel) {}
void Poller::removeChannel(Channel *channel) {}
void Poller::fillActiveChannels(int numEvents,
                                ChannelList *activaChannels) const {}
void Poller::update(int operation, Channel *channel) {}
// Fake Poller end

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

TEST_CASE("Enable and Disable read/write") {
  EventLoop loop;
  Channel channel(&loop, 0);

  channel.enableReading();
  CHECK(channel.isReading());

  channel.enableWriting();
  CHECK(channel.isWriting());

  channel.disableWriting();
  CHECK(not channel.isWriting());

  channel.disableAll();
  CHECK(channel.isNoneEvent());
}
