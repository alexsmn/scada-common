#pragma once

#include <boost/asio/steady_timer.hpp>

template <class Task>
inline void PostDelayedTask(boost::asio::io_context& io_context,
                            std::chrono::nanoseconds delay,
                            Task&& task) {
  if (delay == std::chrono::nanoseconds::zero()) {
    io_context.post(std::forward<Task>(task));
    return;
  }

  auto timer = std::make_shared<boost::asio::steady_timer>(io_context);
  timer->expires_from_now(delay);
  timer->async_wait([timer, task = std::forward<Task>(task)](
                        boost::system::error_code ec) mutable {
    if (ec != boost::asio::error::operation_aborted)
      task();
  });
}
