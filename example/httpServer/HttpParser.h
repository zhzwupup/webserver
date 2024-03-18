#pragma once

#include "Buffer.h"
#include "HttpMessage.h"
#include "http_parser.h"
#include "spdlog/spdlog.h"
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>

extern std::shared_ptr<spdlog::logger> logger;

int on_url(http_parser *p, const char *at, size_t length);
int on_header_field(http_parser *p, const char *at, size_t length);
int on_header_value(http_parser *p, const char *at, size_t length);
int on_body(http_parser *p, const char *at, size_t length);
int on_message_complete(http_parser *p);

class HttpParser {
public:
  HttpParser() {
    http_parser_settings_init(settings_.get());
    settings_->on_header_field = on_header_field;
    settings_->on_header_value = on_header_value;
    settings_->on_body = on_body;
    settings_->on_message_complete = on_message_complete;
  }

  void onHeaderField(const char *at, size_t length) {
    headerFieldTemp_ = std::string(at, length);
  }

  void onHeaderValue(const char *at, size_t length) {
    if (headerFieldTemp_.has_value()) {
      message_->setHeader(headerFieldTemp_.value(), std::string(at, length));
      headerFieldTemp_.reset();
    }
  }

  void onBody(const char *at, size_t length) {
    message_->setContent(std::string(at, length));
  }

  void onMessageComplete() {
    message_->setVersion(parser_->http_major, parser_->http_minor);
    complete_ = true;
    setLeftInformation();
  }

  bool executeParser(Buffer *buf) {
    logger->debug("buf->readableBytes(): {}", buf->readableBytes());
    logger->debug("buf: {}", std::string(buf->peek(), buf->readableBytes()));
    size_t parsed = http_parser_execute(parser_.get(), settings_.get(),
                                        buf->peek(), buf->readableBytes());
    if (HTTP_PARSER_ERRNO(parser_.get()) == HPE_OK) {
      buf->retrieve(parsed);
      return true;
    }
    return false;
  }

  void reset() {
    complete_ = false;
    message_.reset();
    headerFieldTemp_ = {};
  }

  bool complete() { return complete_; }

protected:
  virtual void setLeftInformation() = 0;
  std::unique_ptr<http_parser> parser_ = std::make_unique<http_parser>();
  std::unique_ptr<http_parser_settings> settings_ =
      std::make_unique<http_parser_settings>();
  std::unique_ptr<HttpMessage> message_ = nullptr;
  std::optional<std::string> headerFieldTemp_ = {};

  bool complete_ = false;
};

class HttpRequestParser : public HttpParser {
public:
  HttpRequestParser() {
    http_parser_init(parser_.get(), HTTP_REQUEST);
    parser_->data = this;
    settings_->on_url = on_url;
    message_.reset(new HttpRequest);
  }

  void onUrl(const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(message_.get());
    logger->debug("uri: {}", std::string(at, length));
    request->setUri(std::string(at, length));
  }

  virtual void setLeftInformation() override {
    HttpRequest *request = static_cast<HttpRequest *>(message_.get());
    request->setMethod(static_cast<http_method>(parser_->method));
  }

  HttpRequest *getRequest() {
    return static_cast<HttpRequest *>(message_.get());
  }
};
