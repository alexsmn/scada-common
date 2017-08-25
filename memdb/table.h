#pragma once

#include "memdb/descriptor.h"

#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace memdb {

class TableAccessor;
class TableUpdateInfo;

struct TableScheme {
  typedef std::vector<Field> Fields;
  
  TableScheme()
      : original_version(0),
        record_size(0) {
  }
  
  unsigned original_version;
  size_t record_size;
  Fields fields;
};

class Table {
 public:
  typedef std::map<RID, void*> RecordMap;
  typedef RecordMap::iterator iterator;
  typedef RecordMap::const_iterator const_iterator;
 
  explicit Table(const Descriptor& descriptor);
  virtual ~Table();

  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;

  void set_original_version(unsigned version) { original_version_ = version; }
  void SetAccessor(TableAccessor* accessor);
  
  const Descriptor& descriptor() const { return descriptor_; }
  TableAccessor* accessor() const { return accessor_.get(); }
  bool empty() const { return records_.empty(); }
  size_t record_count() const { return records_.size(); }
  iterator begin() { return records_.begin(); }
  const_iterator begin() const { return records_.begin(); }
  iterator end() { return records_.end(); }
  const_iterator end() const { return records_.end(); }
  
  unsigned original_version() const { return original_version_; }

  virtual void NewRecord(void* rec) const;

  RID GenerateID();

  // Returns nullptr on failure.
  void* Insert(const void* data);
  void* Insert(RID rid, size_t field_count, const FieldValue* field_values);
  // Returns false on error.
  bool Update(RID rid, size_t field_count, const FieldValue* field_values);
  bool Delete(RID rid);
  const void* FindRecord(RID rid) const;

  // Returns nullptr on failure.
  void* InternalInsert(RecordUID uid, const void* rec);
  
  size_t PackRecord(const void* rec, void* buffer, size_t size) const;
  bool UnpackRecord(void* rec, const void* buffer, size_t size) const;

  TID id;

protected:
  // Allocation in memory:	[ RECORD | MEM_DATA ]
  // Allocation in file:		[ FILE_DATA | RECORD ]

  RecordUID& record_uid(void* rec) {
    assert(rec);
    assert(memory_record_size_ == descriptor_.record_size() + sizeof(RecordUID));
    return *reinterpret_cast<RecordUID*>(
        reinterpret_cast<uint8_t*>(rec) + descriptor_.record_size());
  }

  const RecordUID record_uid(const void* rec) const {
    return const_cast<Table*>(this)->record_uid(
        const_cast<void*>(rec));
  }

  const Descriptor& descriptor_;

  size_t memory_record_size_;  // size of record in memory

  RecordMap records_;

  std::unique_ptr<TableAccessor> accessor_;
  unsigned original_version_;

  RID next_rid;
};

inline const void* Table::FindRecord(RID rid) const {
  assert(this);
  RecordMap::const_iterator i = records_.find(rid);
  if (i == records_.end())
    return NULL;
  void* rec = i->second;
  assert(GetRecordID(rec) == rid);
  return rec;
}

const void* GetRecordFieldPtr(const Descriptor& tbl, const void* rec, PID pid);

} // namespace memdb
