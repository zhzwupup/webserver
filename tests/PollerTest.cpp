#include "Poller.h"
#include "doctest.h"
#include <cstdint>
#include <iterator>
#include <spdlog/common.h>
#include <sys/eventfd.h>

#include <spdlog/sinks/stdout_sinks.h>
#include <thread>

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(5);
auto console = std::make_shared<spdlog::sinks::stdout_sink_mt>();
std::vector<spdlog::sink_ptr> sinks;
std::shared_ptr<spdlog::logger> logger;

// Fake EventLoop
EventLoop::EventLoop()
    : m_thread_id(std::this_thread::get_id()), m_looping(false), m_quit(false),
      poller_(std::make_unique<Poller>(this)),
      wakeupFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {}
EventLoop::~EventLoop() {}
// Fake EventLoop end

// Fake Channel
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    : m_loop(loop), m_fd(fd), m_events(0), m_revents(0), m_state(-1),
      tied_(false) {}
Channel::~Channel() {}
void Channel::update() {}
// Fake Channel end

TEST_CASE("poll") {
  ringbuffer_sink->set_pattern("%v");
  sinks.push_back(ringbuffer_sink);
  // sinks.push_back(console);
  logger = std::make_shared<spdlog::logger>("ringbuffer_logger",
                                            std::begin(sinks), std::end(sinks));
  logger->set_level(spdlog::level::debug);

  using ChannelList = std::vector<Channel *>;
  EventLoop loop;
  Poller poller{&loop};
  ChannelList activeChannels;
  SUBCASE("Timeout") {

    activeChannels.clear();
    poller.poll(1000, &activeChannels);
    std::string expected{"timeout!\n"};
    std::string text = ringbuffer_sink->last_formatted().back();
    CHECK(expected == text);
  }

  SUBCASE("Add a channel and be polled") {
    Channel channel{&loop, eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)};
    channel.enableReading();
    poller.updateChannel(&channel);
    int efd = channel.fd();

    using namespace std::chrono_literals;
    auto writer = std::thread([efd]() {
      std::this_thread::sleep_for(1s);
      uint64_t one = 1;
      ssize_t n = write(efd, &one, sizeof(one));
    });

    activeChannels.clear();
    // 从阻塞中返回，就说明读取到事件
    poller.poll(-1, &activeChannels);
    writer.join();
  }
}
