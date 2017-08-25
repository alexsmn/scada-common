#include "memdb/memory_db.h"

namespace memdb {

MemoryDB::MemoryDB() {
}

MemoryDB::~MemoryDB() {
}

void MemoryDB::AddTable(std::unique_ptr<Table> table) {
  assert(table);
  assert(table->id >= 0 && table->id < sizeof(tables_) / sizeof(tables_[0]));
  assert(!tables_[table->id]);
  
  tables_[table->id] = std::move(table);
}

} // namespace memdb
