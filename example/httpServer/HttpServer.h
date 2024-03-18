#include "Buffer.h"
#include "Callback.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpMessage.h"
#include "InetAddress.h"
#include "TcpServer.h"
#include "http_parser.h"

using HttpRequestHandler =
    std::function<void(const HttpRequest *, std::shared_ptr<HttpResponse>)>;

class HttpServer {
public:
  HttpServer(EventLoop *loop, const InetAddress &address)
      : server_(loop, address, "http-server") {
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
  }

  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  void start() { server_.start(); }
  void handle(HttpContextPtr, const TcpConnectionPtr &);
  void RegisterHttpRequestHandler(const std::string &path, std::string method,
                                  const HttpRequestHandler callback) {
    request_handlers_[path].insert(std::make_pair(method, std::move(callback)));
  }

private:
  void onConnection(const TcpConnectionPtr &conn);
  void onMessage(const TcpConnectionPtr &conn, Buffer *buf);

  TcpServer server_;
  std::map<std::string, std::map<std::string, HttpRequestHandler>>
      request_handlers_;
};
