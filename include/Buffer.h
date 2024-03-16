#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

class Buffer {
public:
  static constexpr size_t kCheapPrepend = 8;
  static constexpr size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {}

  ssize_t ReadFd(int fd, int *saveErrno);
  ssize_t WriteFd(int fd, int *saveErrno);

  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }
  void retrieve(size_t len) {
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }
  size_t readableBytes() const { return writerIndex_ - readerIndex_; }

  size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  size_t prependableBytes() const { return readerIndex_; }

  const char *peek() const { return begin() + readerIndex_; }

  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
  }

  void append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  char *beginWrite() { return begin() + writerIndex_; }

  const char *beginWrite() const { return begin() + writerIndex_; }

private:
  char *begin() { return &(*buffer_.begin()); }

  const char *begin() const { return &(*buffer_.begin()); }

  void makeSpace(int len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      buffer_.resize(writerIndex_ + len);
    } else {
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
