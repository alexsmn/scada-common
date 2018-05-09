#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

class Timer {
 public:
  using Period = std::chrono::nanoseconds;

  explicit Timer(boost::asio::io_context& io_context)
      : io_context_{io_context} {}

  bool started() const { return core_ != nullptr; }

  template <class Callback>
  void StartOne(Period period, Callback&& callback) {
    core_ = std::make_shared<CoreImpl<false, Callback>>(
        io_context_, period, std::forward<Callback>(callback));
    core_->Start();
  }

  template <class Callback>
  void StartRepeating(Period period, Callback&& callback) {
    core_ = std::make_shared<CoreImpl<true, Callback>>(
        io_context_, period, std::forward<Callback>(callback));
    core_->Start();
  }

  void Stop() { core_ = nullptr; }

 private:
  struct Core {
    virtual ~Core() {}
    virtual void Start() = 0;
  };

  template <bool kRepeating, class Callback>
  struct CoreImpl
      : public Core,
        public std::enable_shared_from_this<CoreImpl<kRepeating, Callback>> {
    CoreImpl(boost::asio::io_context& io_context,
             Period period,
             Callback&& callback)
        : timer_{io_context},
          period_{period},
          callback_{std::forward<Callback>(callback)} {}

    ~CoreImpl() { timer_.cancel(); }

    virtual void Start() override {
      timer_.expires_from_now(period_);
      timer_.async_wait([weak_core = this->weak_from_this()](boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted)
          return;
        if (auto core = weak_core.lock())
          core->callback_();
        if (kRepeating) {
          if (auto core = weak_core.lock())
            core->Start();
        }
      });
    }

    boost::asio::steady_timer timer_;
    const Period period_;
    Callback callback_;
  };

  boost::asio::io_context& io_context_;
  std::shared_ptr<Core> core_;
};
