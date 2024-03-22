#include "Callback.h"
#include "EventLoop.h"
#include "HttpMessage.h"
#include "InetAddress.h"
#include "TcpContext.h"
#include "TcpServer.h"

using HttpRequestHandler =
    std::function<void(const HttpRequest *, std::shared_ptr<HttpResponse>)>;

class WebSocketServer {
public:
  using TextMessageCallback =
      std::function<void(const TcpConnectionPtr &, const std::string &, bool)>;
  using BinaryMessageCallback =
      std::function<void(const TcpConnectionPtr &, const char *, size_t, bool)>;

  WebSocketServer(EventLoop *loop, const InetAddress &addr,
                  const std::string name);

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  void setTextMessageCallback(const TextMessageCallback &cb) {
    textMessageCallback_ = cb;
  }

  void setBinaryMessageCallback(const BinaryMessageCallback &cb) {
    binaryMessageCallback_ = cb;
  }

  void start() { server_.start(); }

  void setThreadNums(int numThreads) { server_.setThreadNum(numThreads); }

  // void handle(const TcpContextPtr &, const TcpConnectionPtr &);
  void handleHttp(const TcpContextPtr &, const TcpConnectionPtr &);
  void handleWs(const TcpContextPtr &, const TcpConnectionPtr &, Buffer *buf);

  void RegisterHttpRequestHandler(const std::string &path, std::string method,
                                  const HttpRequestHandler callback) {
    request_handlers_[path].insert(std::make_pair(method, std::move(callback)));
  }

private:
  void onConnection(const TcpConnectionPtr &conn);
  void onMessage(const TcpConnectionPtr &conn, Buffer *buf);

  // void sentText(const TcpConnectionPtr &conn, const std::string &data,
  //               bool mask = false);
  // void sendBinary(const TcpConnectionPtr &conn, const char *data, size_t len,
  //                 bool mask = false);

  void onHandShake(const TcpConnectionPtr &conn, const HttpRequest &req);

  bool isHandShakeReq(const HttpRequest &req);
  void responseHandShake(Buffer *buf, const std::string &websocketKey);

  TcpServer server_;
  ConnectionCallback connectionCallback_;
  TextMessageCallback textMessageCallback_;
  BinaryMessageCallback binaryMessageCallback_;
  std::map<std::string, std::map<std::string, HttpRequestHandler>>
      request_handlers_;
};
