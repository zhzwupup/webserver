#include "WebSocketServer.h"
#include "Buffer.h"
#include "Callback.h"
#include "EventLoop.h"
#include "TcpContext.h"
#include "TcpServer.h"
#include <cstdint>
#include <sstream>

WebSocketServer::WebSocketServer(EventLoop *loop, const InetAddress &addr,
                                 const std::string name)
    : server_(loop, addr, name) {
  logger->info("WebSocket Server Run on port {}", addr.toPort());
  server_.setConnectionCallback(
      std::bind(&WebSocketServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(std::bind(&WebSocketServer::onMessage, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
}

void WebSocketServer::onConnection(const TcpConnectionPtr &conn) {
  if (conn->connected()) {
    conn->setContext(std::make_shared<TcpContext>());
  } else {
    conn->setContext(nullptr);
  }
}

void WebSocketServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  TcpContextPtr context = std::any_cast<TcpContextPtr>(conn->getContext());
  if (not context->handshaked()) {
    handleHttp(conn, buf);
  } else {
    handleWs(conn, buf);
  }
}

void sendCloseResponse(http_status statusCode,
                       std::shared_ptr<HttpResponse> response,
                       const TcpConnectionPtr &conn, Buffer *buf) {
  response->setStatusCode(statusCode);
  response->setHeader("Connection", "Close");
  response->appendToBuffer(buf);
  conn->send(buf);
  conn->shutdown();

  return;
}

void WebSocketServer::handleHttp(const TcpConnectionPtr &conn, Buffer *buf) {

  TcpContextPtr context = std::any_cast<TcpContextPtr>(conn->getContext());
  std::shared_ptr<HttpContext> httpCtx = context->httpContext();

  if (not httpCtx->parse(buf)) {
    logger->trace("close connection");
    return;
  }

  if (httpCtx->upgrade()) {
    // 发送握手响应
    Buffer out;
    HttpRequest *request = context->httpContext()->getRequest();

    logger->debug("Sec-WebSocket-Key: {}",
                  request->header("Sec-WebSocket-Key").value());
    responseHandShake(&out, request->header("Sec-WebSocket-Key").value());
    conn->send(&out);
    context->setHandshaked();
    return;
  }

  if (not httpCtx->ready()) {
    return;
  }

  // 正常 http 请求处理
  HttpRequest *request = context->httpContext()->getRequest();
  auto response = context->httpContext()->getResponse();
  auto it = request_handlers_.find(request->uri());
  Buffer output;

  if (it == request_handlers_.end()) {
    sendCloseResponse(HTTP_STATUS_NOT_FOUND, response, conn, &output);
    return;
  }

  auto callback_it = it->second.find(request->methodString());
  if (callback_it == it->second.end()) {
    sendCloseResponse(HTTP_STATUS_METHOD_NOT_ALLOWED, response, conn, &output);
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
  return;
}

void WebSocketServer::responseHandShake(Buffer *buf,
                                        const std::string &websocketKey) {
  buf->append("HTTP/1.1 101 Switching Protocols\r\n"
              "Upgrade: websocket\r\n"
              "Connection: Upgrade\r\n"
              // "Sec-WebSocket-Version: 13\r\n"
  );
  // sec-websocket-accept
  buf->append("sec-websocket-accept: " + getSecWebsocketAccept(websocketKey) +
              "\r\n\r\n");
}

void WebSocketServer::handleWs(const TcpConnectionPtr &conn, Buffer *buf) {
  TcpContextPtr context = std::any_cast<TcpContextPtr>(conn->getContext());
  std::shared_ptr<WebSocketContext> wsCtx = context->wsContext();
  if (not wsCtx->decode(buf)) {
    logger->info("invalid websocket message");
    conn->shutdown();
  } else {
    if (wsCtx->ready()) {
      WebSocketData &reqData = wsCtx->reqData();
      uint8_t opcode = reqData.opcode();
      if (opcode == frame::opcode::PING) {
        Buffer output;
        WebSocketContext::makePongFrame(&output);
        conn->send(&output);
      } else if (opcode == frame::opcode::PONG or
                 opcode == frame::opcode::TEXT or
                 opcode == frame::opcode::BINARY) {
        // TODO 定时器

        if (opcode == frame::opcode::TEXT) {
          logger->debug("websocket payload: {}",
                        wsCtx->reqData().payload().content());
          // textMessageCallback_(conn,
          //                      std::string(reqData.payload().peek(),
          //                                  reqData.payload().readableBytes()),
          //                      false);
          handleWsTextMessage(conn,
                              std::string(reqData.payload().peek(),
                                          reqData.payload().readableBytes()),
                              false);
        }
        if (opcode == frame::opcode::BINARY and binaryMessageCallback_) {
          binaryMessageCallback_(conn, reqData.payload().peek(),
                                 reqData.payload().readableBytes(), false);
        }
      } else if (opcode == frame::opcode::CLOSE) {
        conn->shutdown();
      }
      wsCtx->reset();
    }
  }
  return;
}

void WebSocketServer::handleWsTextMessage(const TcpConnectionPtr &conn,
                                          const std::string &message,
                                          bool mask) {
  auto callback_it = ws_text_handlers_.find(message);
  if (callback_it == ws_text_handlers_.end()) {
    Buffer payload;
    std::ostringstream oss;
    oss << "server received message:\n" << message;
    payload.append(oss.str());
    Buffer buf;
    WebSocketContext::makeFrame(&buf, frame::opcode::TEXT, mask, &payload);
    conn->send(&buf);
    return;
  }
  callback_it->second(conn, mask);
  return;
}
