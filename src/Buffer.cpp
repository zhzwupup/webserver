
#include "Buffer.h"
#include <bits/types/struct_iovec.h>
#include <cerrno>
#include <cstdio>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::ReadFd(int fd, int *saveErrno) {
  char extrebuf[65536] = {0};

  struct iovec vec[2];
  const size_t writable = writableBytes();

  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;

  vec[1].iov_base = extrebuf;
  vec[1].iov_len = sizeof(extrebuf);

  const int iovcnt = (writable < sizeof(extrebuf) ? 2 : 1);
  const ssize_t n = readv(fd, vec, iovcnt);

  if (n < 0) {
    *saveErrno = errno;
  } else if (n < writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrebuf, n - writable);
  }
  return n;
}

ssize_t Buffer::WriteFd(int fd, int *saveErrno) {
  ssize_t n = write(fd, peek(), readableBytes());
  if (n < 0) {
    *saveErrno = errno;
  }
  return n;
}
