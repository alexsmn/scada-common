#pragma once

namespace memdb {

class FieldValue {
 public:
  FieldValue() { }
  
  FieldValue(unsigned field, size_t size, const void* data)
      : field(field),
        size(size),
        data(data) {
  }

  unsigned field;
  size_t size;
  const void* data;
};

} // namespace memdb
