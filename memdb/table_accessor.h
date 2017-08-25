#pragma once

#include "memdb/types.h"

namespace memdb {

class Table;

class TableAccessor {
 public:
  explicit TableAccessor(Table& table) : table_(table) { }
  virtual ~TableAccessor() { }

  TableAccessor(const TableAccessor&) = delete;
  TableAccessor& operator=(const TableAccessor&) = delete;
  
  Table& table() const { return table_; }

  virtual bool Open() = 0;
  virtual std::pair<bool, RecordUID> Insert(const void* data) = 0;
  virtual bool Update(RecordUID uid, const void* data) = 0;
  virtual bool Delete(RecordUID uid) = 0;
  
 private:
  Table& table_;
};

} // namespace memdb
