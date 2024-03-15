#include "Thread.h"
#include <atomic>
#include <cstdio>
#include <memory>
#include <semaphore>
#include <string_view>
#include <thread>

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string_view &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)),
      name_(name) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ and not joined_) {
    thread_->detach();
  }
}

void Thread::start() {
  started_ = true;
  std::counting_semaphore thrd_to_main{0};

  thread_ = std::make_shared<std::thread>([&thrd_to_main, this]() {
    thrd_to_main.release();
    func_();
  });

  thrd_to_main.acquire();
  tid_ = thread_->get_id();
}

void Thread::join() {
  joined_ = true;
  thread_->join();
}

void Thread::setDefaultName() {
  int num = ++numCreated_;
  if (name_.empty()) {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = buf;
  }
}
