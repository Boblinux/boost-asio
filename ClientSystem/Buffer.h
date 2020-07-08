#ifndef REACTOR_NET_BUFFER_H
#define REACTOR_NET_BUFFER_H

#include "copyable.h"
#include <memory.h>
#include <sys/uio.h>
#include <algorithm>
#include <string>
#include <vector>
#include <assert.h>

namespace Reactor
{

class Buffer : public Reactor::copyable
{
 public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
  {
  }

  size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  size_t prependableBytes() const { return readerIndex_; }
  const char* peek() const { return &*buffer_.begin() + readerIndex_; }

  void retrieve(size_t len)
  {
    readerIndex_ += len;
  }

  void retrieveAll()
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  std::string readAsString()
  {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
  }

  std::string readAsBlock(size_t size)
  {
    std::string str(peek(), size);
    readerIndex_ += size;
    return str;
  }

  void append(const char* data, size_t len)
  {
    ensureWritableBytes(len);
    std::copy(data, data+len, &*buffer_.begin() + writerIndex_);
    writerIndex_ += len;
  }

  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
  }

  void prepend(const void* data, size_t len)
  {
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, &*buffer_.begin()+readerIndex_);
  }

/// Read data directly into buffer.
ssize_t readFd(int fd)
{
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = &*buffer_.begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  const ssize_t n = readv(fd, vec, 2);
  if (n < 0) {
    ;
  } else if (static_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

 private:

  void makeSpace(size_t len)
  {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_+len);
    }
    else
    {
      // move readable data to the front, make space inside buffer
      size_t readable = readableBytes();
      std::copy(&*buffer_.begin()+readerIndex_,
                &*buffer_.begin()+writerIndex_,
                &*buffer_.begin()+kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

 private:
  std::vector<char>   buffer_;
  size_t              readerIndex_;
  size_t              writerIndex_;
};

}

#endif
