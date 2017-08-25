#pragma once

#include "memdb/table.h"

namespace memdb {

template<class Record>
class TableImpl : public Table {
 public:
  typedef Record Record;

  TableImpl(TID tid)
      : Table(Record::GetDescriptor()) {
    this->id = tid;
    CheckFields();
  }

  const Record* FindRecord(RID rid) const {
    return reinterpret_cast<const Record*>(Table::FindRecord(rid));
  }

  // Table
  virtual void NewRecord(void* rec) const {
    assert(rec);
    reinterpret_cast<Record*>(rec)->Clear();
  }

 private:
  void CheckFields() {
#ifdef _DEBUG
    // Check field types and sizes.
    for (size_t i = 0; i < descriptor().field_count(); ++i) {
      DBType type = descriptor().field(i).type;
      size_t size = descriptor().field(i).size;
      switch (type) {
        case DB_BOOL:
        case DB_UINT8:
          assert(size == 1);
          break;
        case DB_UINT16:
          assert(size == 2);
          break;
        case DB_UINT32:
          assert(size == 4);
          break;
        case DB_FLOAT:
          assert(size == 8);
          break;
        case DB_TID:
          assert(size == sizeof(TID));
          break;
        case DB_STRING:
          assert(size >= 2);
          break;
        default:
          assert(false);
          break;
      }
    }

    // Find identical fields.
    if (descriptor().field_count() > 0) {
      for (size_t i = 0; i < descriptor().field_count() - 1; ++i) {
        PID pid1 = descriptor().field(i).pid;
        for (size_t j = i + 1; j < descriptor().field_count(); ++j) {
          PID pid2 = descriptor().field(j).pid;
          assert(pid1 != pid2);
        }
      }
    }
#endif
  }
};

} // namespace memdb
