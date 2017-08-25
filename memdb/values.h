#pragma once

#include "memdb/field_value.h"
#include "memdb/types.h"

#include <vector>

namespace memdb {

class Descriptor;

class Values {
 public:
  explicit Values(const Descriptor& table) : table_(table) { }

  Values(const Values&) = delete;
  Values& operator=(const Values&) = delete;

  const std::vector<FieldValue>& values() const { return values_; }

  void AddValue(const FieldValue& value);

  void SetBuffer(PID pid, const void* data, size_t size);
  bool DoesExist(PID pid) const;

  template<typename T>
  void Set(PID pid, const T& val) {
    SetBuffer(pid, &val, sizeof(T));
  }
  
 private:
  const Descriptor& table_;

  std::vector<FieldValue> values_;
  std::vector<std::vector<char> > datum_;
};

} // namespace memdb
