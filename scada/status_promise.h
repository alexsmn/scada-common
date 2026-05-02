#pragma once

#include "base/promise.h"
#include "scada/status_exception.h"
#include "scada/status_or.h"

#include <D:/tc/third_party/promise.hpp/headers/promise.hpp/promise.hpp>

namespace scada {

inline [[nodiscard]] promise<void> MakeRejectedStatusPromise(Status status) {
  return make_rejected_promise<>(status_exception{std::move(status)});
}

template <class T>
inline [[nodiscard]] promise<T> MakeRejectedStatusPromise(Status status) {
  return make_rejected_promise<T>(status_exception{std::move(status)});
}

inline [[nodiscard]] promise<void> MakeCompletedStatusPromise(Status status) {
  return status ? make_resolved_promise()
                : make_rejected_promise<>(status_exception{std::move(status)});
}

template <class T>
inline [[nodiscard]] promise<T> MakeCompletedStatusPromise(
    const StatusOr<T>& value) {
  return value.ok()
             ? make_resolved_promise(value.value())
             : make_rejected_promise<T>(status_exception{value.status()});
}

template <class T>
inline [[nodiscard]] promise<T> MakeCompletedStatusPromise(
    StatusOr<T>&& value) {
  return value.ok() ? make_resolved_promise<T>(std::move(*value))
                    : make_rejected_promise<T>(
                          status_exception{std::move(value).status()});
}

inline [[nodiscard]] promise<void> ToStatusPromise(promise<Status> promise) {
  return promise.then([](const Status& status) {
    return status.bad() ? MakeRejectedStatusPromise(status)
                        : make_resolved_promise();
  });
}

inline [[nodiscard]] promise<void> ToStatusPromise(
    promise<StatusCode> promise) {
  return promise.then([](StatusCode status_code) {
    return IsBad(status_code) ? MakeRejectedStatusPromise(status_code)
                              : make_resolved_promise();
  });
}

template <class T>
inline [[nodiscard]] promise<StatusCode> ToExplicitStatusCodePromise(
    promise<T> promise) {
  return promise.then(
      [](const T&) { return StatusCode::Good; },
      [](const std::exception_ptr& e) { return GetExceptionStatus(e).code(); });
}

template <>
inline [[nodiscard]] promise<StatusCode> ToExplicitStatusCodePromise(
    promise<void> promise) {
  return promise.then(
      [] { return StatusCode::Good; },
      [](const std::exception_ptr& e) { return GetExceptionStatus(e).code(); });
}

template <class T>
inline void RejectStatusPromise(promise<T>& promise, Status bad_status) {
  assert(!bad_status);
  promise.reject(scada::status_exception{std::move(bad_status)});
}

inline void RejectStatusPromise(promise<void>& promise, Status bad_status) {
  assert(!bad_status);
  promise.reject(scada::status_exception{std::move(bad_status)});
}

inline void CompleteStatusPromise(promise<void>& promise, Status status) {
  if (status) {
    promise.resolve();
  } else {
    RejectStatusPromise(promise, std::move(status));
  }
}

template <class T>
inline void CompleteStatusPromise(promise<T>& promise, StatusOr<T>&& value) {
  if (value.ok()) {
    promise.resolve(std::move(*value));
  } else {
    RejectStatusPromise(promise, std::move(value).status());
  }
}

template <class T>
inline void CompleteStatusPromise(promise<T>& promise,
                                  const StatusOr<T>& value) {
  if (value.ok()) {
    promise.resolve(*value);
  } else {
    RejectStatusPromise(promise, value.status());
  }
}

// Properly assigns status callback to a status promise. The callback is invoked
// when status exception is triggered.
//
// WARNING: The current implementation requires a copyable callback since it has
// to be copied both to `.then` and to `.except` handlers.
template <class T, class Callback>
inline promise<void> BindStatusCallback(promise<T> promise,
                                        const Callback& callback) {
  // Explicit `callback` copy is required to make it mutable. Mutability is
  // needed e.g. when capturing promises.
  return promise.then(callback, [callback](std::exception_ptr e) {
    callback({.status = GetExceptionStatus(e)});
  });
}

// WARNING: The current implementation requires a copyable callback since it has
// to be copied both to `.then` and to `.except` handlers.
template <class Callback>
inline promise<void> BindStatusCallback(promise<void> promise,
                                        const Callback& callback) {
  // Explicit `callback` copy is required to make it mutable. Mutability is
  // needed e.g. when capturing promises.
  // Explicit `callback` copy is required to make it mutable. Mutability is
  // needed e.g. when capturing promises.
  return promise.then([callback]() mutable { callback(StatusCode::Good); },
                      [callback](std::exception_ptr e) mutable {
                        callback(GetExceptionStatus(e));
                      });
}

// A callback to pass to a promise:
// Example:
//   promise<> p;
//   session_proxy_.Connect("port=3000", u"username", u"password",
//     MakeStatusPromiseCallback(p));
inline [[nodiscard]] auto MakeStatusPromiseCallback(promise<void> promise) {
  return [promise = std::move(promise)](Status status) mutable {
    if (status.bad()) {
      promise.reject(status_exception{std::move(status)});
    } else {
      promise.resolve();
    }
  };
}

template <class T>
inline [[nodiscard]] auto MakeStatusPromiseCallback(promise<T> promise) {
  return [promise = std::move(promise)](T&& result) mutable {
    if (!result.status) {
      RejectStatusPromise(promise, std::move(result.status));
    } else {
      promise.resolve(std::move(result));
    }
  };
}

template <class T>
inline [[nodiscard]] auto MakeStatusCodePromiseCallback(promise<T> promise) {
  return [promise = std::move(promise)](T&& result) mutable {
    if (IsBad(result.status_code)) {
      RejectStatusPromise(promise, result.status_code);
    } else {
      promise.resolve(std::move(result));
    }
  };
}

}  // namespace scada
