#pragma once

#include "HttpContext.h"
#include "WebSocketContext.h"
#include <memory>
#include <optional>
class TcpContext {
public:
  void setHandshaked() {
    handshaked_ = true;
    httpContext_->reset();
    httpContext_ = nullptr;
    wsContext_ = std::make_shared<WebSocketContext>();
  }

  bool handshaked() const { return handshaked_; }

  std::shared_ptr<HttpContext> httpContext() const { return httpContext_; }

  std::shared_ptr<WebSocketContext> wsContext() const { return wsContext_; }

private:
  bool handshaked_ = false;
  std::shared_ptr<HttpContext> httpContext_ = std::make_shared<HttpContext>();
  std::shared_ptr<WebSocketContext> wsContext_ = nullptr;
};

using TcpContextPtr = std::shared_ptr<TcpContext>;
