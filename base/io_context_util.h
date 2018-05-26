#pragma once

#include <boost/asio/steady_timer.hpp>

template <class Task>
inline void PostDelayedTask(boost::asio::io_context& io_context,
                            std::chrono::nanoseconds delay,
                            const Task& task) {
  auto timer = std::make_shared<boost::asio::steady_timer>(io_context);
  timer->expires_from_now(delay);
  timer->async_wait([task](boost::system::error_code ec) {
    if (ec != boost::asio::error::operation_aborted)
      task();
  });
}
