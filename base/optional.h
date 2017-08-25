#pragma once

template<typename T>
class Optional {
 public:
  Optional() {}
  Optional(std::nullptr_t) {}
  Optional(const T& value) : value_{value}, initialized_{true} {}

  Optional& operator=(const T& value) {
    value_ = value;
    initialized_ = true;
    return *this;
  }

  bool is_initialized() const { return initialized_; }

  bool operator==(const T& value) const {
    return initialized_ && value_ == value;
  }

  bool operator!=(const T& value) const {
    return !operator==(value);
  }

  const T& get() const {
    assert(initialized_);
    return value_;
  }

 private:
  bool initialized_ = false;
  T value_;
};
