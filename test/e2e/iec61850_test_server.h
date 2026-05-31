#pragma once

#include <atomic>
#include <thread>

namespace client::test {

class Iec61850TestServer {
 public:
  explicit Iec61850TestServer(int port);
  ~Iec61850TestServer();

  bool running() const { return running_; }
  bool failed() const { return failed_; }

 private:
  void Run();

  const int port_ = 0;
  std::atomic<bool> stopping_{false};
  std::atomic<bool> running_{false};
  std::atomic<bool> failed_{false};
  std::thread thread_;
};

}  // namespace client::test
