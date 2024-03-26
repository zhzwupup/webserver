#include "HttpMessage.h"
#include "WebSocketServer.h"

#include "spdlog/sinks/stdout_color_sinks.h"

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

// hidden symbol ... is referenced by DSO 剖析
// https://www.tsingfun.com/it/cpp/2496.html

__attribute__((visibility("default"))) std::shared_ptr<spdlog::logger> logger =
    std::make_shared<spdlog::logger>("console", console_sink);

void sendBinary(const TcpConnectionPtr &conn, const char *data, size_t len,
                bool mask) {
  Buffer payload;
  payload.append(data, len);
  Buffer buf;
  WebSocketContext::makeFrame(&buf, frame::opcode::BINARY, mask, &payload);
  conn->send(&buf);
}

int main() {
  console_sink->set_level(spdlog::level::trace);
  // console_sink->set_pattern("[%^%l%$] %v");
  logger->set_level(spdlog::level::debug);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");

  EventLoop loop;
  WebSocketServer server(&loop, InetAddress(8080), "websocket-server");
  server.setThreadNums(8);

  const auto index = [](const HttpRequest *req,
                        std::shared_ptr<HttpResponse> res) {
    res->setContent("hello world");
    res->setHeader("Content-Type", "text/plain");
  };

  const auto hello = [](const TcpConnectionPtr &conn, bool mask) {
    Buffer payload;
    payload.append("server received hello from client");
    Buffer buf;
    WebSocketContext::makeFrame(&buf, frame::opcode::TEXT, mask, &payload);
    conn->send(&buf);
  };

  server.RegisterHttpRequestHandler("/", "GET", index);
  // server.RegisterHttpRequestHandler("/game", "GET", game);
  // server.setConnectionCallback(onConnection);
  server.RegisterWsTextHandler("hello", hello);
  server.setBinaryMessageCallback(sendBinary);

  server.start();
  loop.loop();
  return 0;
}
