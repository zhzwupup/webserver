#pragma once

#include "Buffer.h"
#include "http_parser.h"
#include "spdlog/spdlog.h"
#include <cstdio>
#include <map>
#include <optional>
#include <sstream>
#include <string>

extern std::shared_ptr<spdlog::logger> logger;

struct HttpVersion {
  unsigned short http_major;
  unsigned short http_minor;
};

class HttpMessage {
public:
  HttpMessage() : version_({1, 1}) {}
  virtual ~HttpMessage() = default;

  void setHeader(const std::string &key, const std::string &value) {
    headers_[key] = std::move(value);
  }
  void removeHeader(const std::string &key) { headers_.erase(key); }
  void clearHeader() { headers_.clear(); }
  void setContent(const std::string &content) {
    content_ = std::move(content);
    // setContentLength();
  }
  void clearContent() {
    content_.clear();
    setContentLength();
  }

  void setVersion(unsigned short http_major, unsigned short http_minor) {
    version_.http_major = http_major;
    version_.http_minor = http_minor;
  }
  HttpVersion version() const { return version_; }
  std::string versionString() const {
    char version[9] = {9};
    snprintf(version, 9, "HTTP/%u.%u", version_.http_major,
             version_.http_minor);
    return std::string(version);
  }
  std::optional<std::string> header(const std::string &key) const {
    std::optional<std::string> value = {};
    if (headers_.find(key) != headers_.end()) {
      value = headers_.at(key);
    }
    return value;
  }
  std::map<std::string, std::string> headers() const { return headers_; }
  std::string content() const { return content_; }
  size_t contentLength() const { return content_.size(); }

private:
  HttpVersion version_{1, 1};
  std::map<std::string, std::string> headers_;
  std::string content_;

  void setContentLength() {
    setHeader("Content-Length", std::to_string(content_.size()));
  }
};

class HttpRequest : public HttpMessage {
public:
  void setMethod(http_method method) { method_ = method; }
  void setUri(const std::string &uri) { uri_ = uri; }

  std::string methodString() const { return http_method_str(method_); }
  std::string uri() const { return uri_; }

private:
  http_method method_ = HTTP_GET;
  std::string uri_ = {};
};

class HttpResponse : public HttpMessage {
public:
  void setStatusCode(http_status statusCode) { statusCode_ = statusCode; }
  http_status statusCode() const { return statusCode_; }
  std::string statusMessage() const { return http_status_str(statusCode_); }
  void appendToBuffer(Buffer *buf) const {
    std::stringstream oss;
    oss << versionString() << " " << statusCode() << " " << statusMessage()
        << "\r\n";

    for (const auto &header : headers()) {
      oss << header.first << ": " << header.second << "\r\n";
    }

    const std::string &body = content();
    if (!body.empty()) {
      oss << "Content-Length: " << body.size() << "\r\n";
    }
    oss << "\r\n";
    oss << body;
    buf->append(oss.str());
  }

private:
  http_status statusCode_ = HTTP_STATUS_OK;
};
