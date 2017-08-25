#include "memdb/values.h"

#include "memdb/descriptor.h"

namespace memdb {

void Values::AddValue(const FieldValue& value) {
  assert(value.field >= 0 && value.field < table_.field_count());
  assert(table_.field(value.field).type == DB_STRING ||
         table_.field(value.field).size == value.size);

  const char* buffer = reinterpret_cast<const char*>(value.data);
  datum_.push_back(std::vector<char>(buffer, buffer + value.size));

  const std::vector<char>& data = datum_.back();
  const char* data_ptr = !data.empty() ? &data[0] : NULL;

  values_.push_back(FieldValue(value.field, value.size, data_ptr));  
}

void Values::SetBuffer(PID pid, const void* data, size_t size) {
  assert(!DoesExist(pid));

  FID fid = table_.FindFID(pid);

  AddValue(FieldValue(fid, size, data));
}

bool Values::DoesExist(PID pid) const {
  for (size_t i = 0; i < values_.size(); ++i) {
    unsigned fid = values_[i].field;
    if (pid == table_.field_pid(fid))
      return true;
  }
  return false;
}

} // namespace memdb
