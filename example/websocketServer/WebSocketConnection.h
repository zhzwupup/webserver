#include "Callback.h"
#include <TcpConnection.h>
#include <any>

class WebSocketConnection {
public:
  void setContext(const std::any &context) { context_ = context; }

  const std::any &getContext() const { return context_; }

private:
  TcpConnectionPtr tcpConn_;
  bool connected_;
  std::any context_;
};

using WebSocketConnectionPtr = std::shared_ptr<WebSocketConnection>;
