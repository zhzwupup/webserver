#include "Callback.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include <unistd.h>

#include "spdlog/sinks/stdout_color_sinks.h"

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
std::shared_ptr<spdlog::logger> logger =
    std::make_shared<spdlog::logger>("console", console_sink);

class EchoServer {
public:
  EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
      : server_(loop, addr, name), loop_(loop) {
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
  }

  void start() { server_.start(); }

private:
  void onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      logger->info("Connection UP : {}",
                   conn->peerAddress().toIpPort().c_str());
    } else {
      logger->info("Connection Down : {}",
                   conn->peerAddress().toIpPort().c_str());
    }
  }

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
    std::string msg = buf->retrieveAllAsString();
    logger->info("{} echo {} bytes", conn->name(), msg.size());
    conn->send(msg);
  }

  EventLoop *loop_;
  TcpServer server_;
};

int main() {

  console_sink->set_level(spdlog::level::trace);
  console_sink->set_pattern("[%^%l%$] %v");
  logger->set_level(spdlog::level::debug);

  logger->info("pid = {}", getpid());
  EventLoop loop;
  InetAddress addr(8080);
  EchoServer server(&loop, addr, "EchoServer");
  server.start();
  loop.loop();

  return 0;
}
