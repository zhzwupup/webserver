#include "HttpParser.h"
#include <cstddef>

int on_url(http_parser *p, const char *at, size_t length) {
  HttpRequestParser *parser = static_cast<HttpRequestParser *>(p->data);
  parser->onUrl(at, length);
  return 0;
}

int on_header_field(http_parser *p, const char *at, size_t length) {
  HttpParser *parser = static_cast<HttpParser *>(p->data);
  parser->onHeaderField(at, length);
  return 0;
}

int on_header_value(http_parser *p, const char *at, size_t length) {
  HttpParser *parser = static_cast<HttpParser *>(p->data);
  parser->onHeaderValue(at, length);
  return 0;
}

int on_body(http_parser *p, const char *at, size_t length) {
  HttpParser *parser = static_cast<HttpParser *>(p->data);
  parser->onBody(at, length);
  return 0;
}

int on_message_complete(http_parser *p) {
  HttpParser *parser = static_cast<HttpParser *>(p->data);
  parser->onMessageComplete();
  return 0;
}
