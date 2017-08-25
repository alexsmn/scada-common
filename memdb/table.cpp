#include "memdb/table.h"

#include "base/bytemsg.h"
#include "memdb/field.h"
#include "memdb/field_value.h"
#include "memdb/table_accessor.h"

namespace memdb {

const void* GetRecordFieldPtr(const Descriptor& tbl, const void* rec, PID pid) {
  assert(rec);
  const Field* field = tbl.FindField(pid);
  return field ? (uint8_t*)rec + field->offset : NULL;
}

inline bool SameField(const Field& field1, const Field& field2) {
  return field1.pid == field2.pid &&
         field1.type == field2.type &&
         field1.size == field2.size;
}

static bool CompareFields(size_t count1, const Field* fields1,
                          size_t count2, const Field* fields2) {
  if (count1 != count2)
    return false;
    
  for (size_t i = 0; i < count1; ++i) {
    if (!SameField(fields1[i], fields2[i]))
      return false;
  }
  return true;
}

// Table

Table::Table(const Descriptor& descriptor)
    : descriptor_(descriptor),
      id(0),
      memory_record_size_(descriptor_.record_size() + sizeof(RecordUID)),
      next_rid(1),
      original_version_(0) {
}

Table::~Table() {
  accessor_.reset();

  for (RecordMap::iterator i = records_.begin(); i != records_.end(); ++i)
    free(i->second);
  records_.clear();
}

void* Table::Insert(const void* data) {
  RID id = GetRecordID(data);

  void*& record = records_[id];
  if (record)
    return nullptr;

  record = malloc(memory_record_size_);
  memcpy(record, data, descriptor_.record_size());

  // save to file
  if (accessor_) {
    auto result = accessor_->Insert(record);
    if (!result.first)
      return nullptr;
    record_uid(record) = result.second;
  }

  return record;
}

void* Table::Insert(RID rid, size_t field_count, const FieldValue* field_values) {
  void* buf = alloca(descriptor_.record_size());
  NewRecord(buf);

  for (size_t i = 0; i < field_count; ++i) {
    const FieldValue& value = field_values[i];
    if (value.field >= descriptor_.field_count())
      return nullptr;

    const Field& info = descriptor_.field(value.field);
    assert(info.offset + info.size <= descriptor_.record_size());
    if (!SetRecordFieldData(buf, info, value.size, value.data))
      return nullptr;
  }

  GetRecordID(buf) = rid;
  
  return Insert(buf);
}

bool Table::Update(RID rid, size_t field_count, const FieldValue* field_values) {
  const void* record = FindRecord(rid);
  if (!record)
    return false;

  bool fields_updated = false;

  for (size_t i = 0; i < field_count; ++i) {
    const FieldValue& value = field_values[i];
    assert(value.field != FID_INVAL);
    if (value.field == FID_ID)
      continue;
    if (value.field >= descriptor_.field_count())
      return false;

    if (!descriptor_.SetValue(const_cast<void*>(record), value))
      return false;

    fields_updated = true;
  }

  // save to file if needed
  if (fields_updated && accessor_) {
    if (!accessor_->Update(record_uid(record), record))
      return false;
  }

  return true;
}

bool Table::Delete(RID rid) {
  RecordMap::iterator i = records_.find(rid);
  if (i == records_.end())
    return false;

  void* rec = i->second;

  if (accessor_ && !accessor_->Delete(record_uid(rec)))
    return false;

  free(rec);
  records_.erase(i);

  return true;
}

void Table::SetAccessor(TableAccessor* accessor) {
  accessor_.reset(accessor);
}

RID Table::GenerateID() {
  while (records_.find(next_rid) != records_.end())
    next_rid++;
  return next_rid++;
}

void Table::NewRecord(void* rec) const {
  memset(rec, 0, descriptor_.record_size());
}

void* Table::InternalInsert(RecordUID uid, const void* nrec) {
  RID rid = GetRecordID(nrec);

  // insert memory rec
  void* rec = malloc(memory_record_size_);
  memcpy(rec, nrec, descriptor_.record_size());
  if (!records_.emplace(rid, rec).second) {
    free(rec);
    return nullptr;
  }

  record_uid(rec) = uid;
  
  return rec;
}

size_t Table::PackRecord(const void* rec, void* buffer, size_t size) const {
  return descriptor_.PackRecord(rec, buffer, size);
}

bool Table::UnpackRecord(void* rec, const void* buffer, size_t size) const {
  NewRecord(rec);
  return descriptor_.UnpackRecord(rec, buffer, size);
}

} // namespace memdb
