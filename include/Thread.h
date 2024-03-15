#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

class Thread {
public:
  using ThreadFunc = std::function<void()>;
  explicit Thread(ThreadFunc func,
                  const std::string_view &name = std::string());
  ~Thread();

  void start();
  void join();

  bool started() const { return started_; }
  std::thread::id tid() const { return tid_; }
  const std::string &name() const { return name_; }

  static int numCreated() { return numCreated_; }

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  std::shared_ptr<std::thread> thread_;
  std::thread::id tid_;

  ThreadFunc func_;
  std::string name_;
  static std::atomic_int32_t numCreated_;
};
