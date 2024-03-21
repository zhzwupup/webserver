#include "HttpServer.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "http_parser.h"
#include <cstdio>
#include <spdlog/spdlog.h>
#include <string>
#include <strings.h>

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
  if (conn->connected()) {
    // SPDLOG_LOGGER_INFO(logger, "new Connection arrived");
    conn->setContext(std::make_shared<HttpContext>());
  } else {
    // SPDLOG_LOGGER_INFO(logger, "Connection closed");
    conn->setContext(nullptr);
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  HttpContextPtr context = std::any_cast<HttpContextPtr>(conn->getContext());
  if (!context->parse(buf)) {
    logger->trace("close connection");
  } else {
    if (context->ready()) {
      char buf[200] = {0};
      snprintf(buf, 199, "Parse request successfully in loop %p",
               (void *)(server_.getLoop()));
      // SPDLOG_LOGGER_INFO(logger, std::string(buf));
      handle(context, conn);
    }
  }
}

void HttpServer::handle(HttpContextPtr context, const TcpConnectionPtr &conn) {
  HttpRequest *request = context->getRequest();
  auto response = context->getResponse();
  auto it = request_handlers_.find(request->uri());
  Buffer output;

  if (it == request_handlers_.end()) {
    response->setStatusCode(HTTP_STATUS_NOT_FOUND);
    response->setHeader("Connection", "Close");
    response->appendToBuffer(&output);
    conn->send(&output);
    conn->shutdown();
    return;
  }

  auto callback_it = it->second.find(request->methodString());
  if (callback_it == it->second.end()) {
    response->setStatusCode(HTTP_STATUS_METHOD_NOT_ALLOWED);
    response->setHeader("Connection", "Close");
    response->appendToBuffer(&output);
    conn->send(&output);
    conn->shutdown();
    return;
  }

  callback_it->second(request, response);

  if (not request->header("Connection").has_value()) {
    request->setHeader("Connection", "close");
  }
  response->setHeader("Connection", request->header("Connection").value());
  response->appendToBuffer(&output);
  conn->send(&output);

  if (strcasecmp(request->header("Connection").value().c_str(), "close") == 0) {
    conn->shutdown();
  }

  // 使用 webbench 只能发送一个 response,因为没有调用 shutdown() , TcpConnection
  // 没有断开 webbench 继续在这个 socket 上 read().
  // 但此时 http server 已经发送完毕，因此不会再往 socket 中写内容
  // 调用 conn->shutdown() 后，webbench 就知道 http server
  // 已经发送完毕，不必阻塞在 read()
  // conn->shutdown();
  return;
}
