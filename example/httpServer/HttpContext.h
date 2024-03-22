#include "HttpMessage.h"
#include "HttpParser.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>

class HttpContext {
public:
  HttpContext() {
    // SPDLOG_LOGGER_INFO(logger, "HttpContext::ctor");
  }
  ~HttpContext() {
    // SPDLOG_LOGGER_INFO(logger, "HttpContext::dtor");
  }

  bool parse(Buffer *buf) { return requestParser_->executeParser(buf); }

  bool ready() { return requestParser_->complete(); }
  HttpRequest *getRequest() { return requestParser_->getRequest(); }
  std::shared_ptr<HttpResponse> getResponse() { return response_; }

  bool upgrade() { return requestParser_->upgrade(); }
  void reset() { requestParser_.reset(); }

private:
  http_parser_type type_ = HTTP_REQUEST;
  std::shared_ptr<HttpRequestParser> requestParser_ =
      std::make_shared<HttpRequestParser>();
  std::shared_ptr<HttpResponse> response_ = std::make_shared<HttpResponse>();
};

using HttpContextPtr = std::shared_ptr<HttpContext>;
