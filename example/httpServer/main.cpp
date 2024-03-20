#include "HttpServer.h"

#include "spdlog/sinks/stdout_color_sinks.h"

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

// hidden symbol ... is referenced by DSO 剖析
// https://www.tsingfun.com/it/cpp/2496.html

__attribute__((visibility("default"))) std::shared_ptr<spdlog::logger> logger =
    std::make_shared<spdlog::logger>("console", console_sink);

int main() {
  console_sink->set_level(spdlog::level::trace);
  // console_sink->set_pattern("[%^%l%$] %v");
  logger->set_level(spdlog::level::debug);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");

  EventLoop loop;
  HttpServer server(&loop, InetAddress(8080));
  server.setThreadNum(8);

  const auto index = [](const HttpRequest *req,
                        std::shared_ptr<HttpResponse> res) {
    res->setContent("hello world");
    res->setHeader("Content-Type", "text/plain");
  };

  const auto send_json = [](const HttpRequest *req,
                            std::shared_ptr<HttpResponse> res) {
    res->setHeader("Content-Type", "application/json");
    res->setContent(R"({"name":"Jack","gender":"male"})");
  };

  const auto send_html = [](const HttpRequest *req,
                            std::shared_ptr<HttpResponse> res) {
    res->setHeader("Content-Type", "text/html");
    res->setContent(R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>send html</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
  <h1>Send Html</h1>
  </body>
</html>)");
  };

  server.RegisterHttpRequestHandler("/", "GET", index);
  server.RegisterHttpRequestHandler("/user", "GET", send_json);
  server.RegisterHttpRequestHandler("/test.html", "GET", send_html);

  server.start();
  loop.loop();
  return 0;
}
