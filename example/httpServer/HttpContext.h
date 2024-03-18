#include "HttpMessage.h"
#include "HttpParser.h"

#include <memory>
#include <sstream>

class HttpContext {
public:
  bool parse(Buffer *buf) { return requestParser_->executeParser(buf); }

  bool ready() { return requestParser_->complete(); }
  HttpRequest *getRequest() { return requestParser_->getRequest(); }
  std::shared_ptr<HttpResponse> getResponse() { return response_; }

  void reset() { requestParser_.reset(); }

private:
  http_parser_type type_ = HTTP_REQUEST;
  std::shared_ptr<HttpRequestParser> requestParser_ =
      std::make_shared<HttpRequestParser>();
  std::shared_ptr<HttpResponse> response_ = std::make_shared<HttpResponse>();
};

using HttpContextPtr = std::shared_ptr<HttpContext>;
