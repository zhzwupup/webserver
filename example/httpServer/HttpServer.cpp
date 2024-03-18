#include "HttpServer.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "http_parser.h"

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
  if (conn->connected()) {
    conn->setContext(std::make_shared<HttpContext>());
  } else {
    conn->setContext(nullptr);
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  HttpContextPtr context = std::any_cast<HttpContextPtr>(conn->getContext());
  if (!context->parse(buf)) {
    logger->trace("close connection");
  } else {
    if (context->ready()) {
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
    return;
  }
  auto callback_it = it->second.find(request->methodString());
  if (callback_it == it->second.end()) {
    response->setStatusCode(HTTP_STATUS_METHOD_NOT_ALLOWED);
    response->setHeader("Connection", "Close");
    response->appendToBuffer(&output);
    conn->send(&output);
    return;
  }
  callback_it->second(request, response);

  if (request->header("Connection").has_value()) {
    response->setHeader("Connection", request->header("Connection").value());
  } else {
    response->setHeader("Connection", "Keep-Alive");
  }
  response->appendToBuffer(&output);
  conn->send(&output);
  return;
}
