#pragma once

#include "memdb/table.h"

#include <memory>
#include <stdexcept>

namespace memdb {

class MemoryDB {
 public:
  MemoryDB();
  virtual ~MemoryDB();

  MemoryDB(const MemoryDB&) = delete;
  MemoryDB& operator=(const MemoryDB&) = delete;
  
  Table& table(TID tid) const;
  Table& write_table(TID tid);
  Table* FindTable(TID tid) const;

  void AddTable(std::unique_ptr<Table> table);
  
 private:
  std::unique_ptr<Table> tables_[32];
};

inline Table& MemoryDB::table(TID tid) const {
  Table* t = FindTable(tid);
  if (!t)
    throw std::runtime_error("Table doesn't exist");
  return *t;
}

inline Table& MemoryDB::write_table(TID tid) {
  Table* t = FindTable(tid);
  if (!t)
    throw std::runtime_error("Table doesn't exist");
  return *t;
}

inline Table* MemoryDB::FindTable(TID tid) const {
  if (tid >= 0 && tid < sizeof(tables_) / sizeof(tables_[0]))
    return tables_[tid].get();
  else
    return NULL;
}

#define BEGIN_DATABASE(DatabaseClass) \
  class DatabaseClass : public memdb::MemoryDB { \
   public: \
    DatabaseClass() { \
  
#define END_DATABASE() \
    } \
  };

#define DEFINE_TABLE(RecordClass, table_id) \
  AddTable(std::make_unique<memdb::TableImpl<RecordClass>>(table_id));

} // namespace memdb
