#pragma once

#include "memdb/types.h"

#include <cassert>
#include <string>

namespace memdb {

const size_t kMaxRecordSize = 2048;

extern const char* str_true;
extern const char* str_false;

// field flags
static const unsigned short FIELD_VIRT = 1 << 0;
static const unsigned short FIELD_NS   = 1 << 1;

struct Field {
  uint8_t pid;
  uint8_t packed_pid;
  size_t offset;
  size_t size;
  DBType type;
  unsigned short flags;
};

// macros

// Routines

inline RID GetRecordID(const void* rec) { assert(rec); return *reinterpret_cast<const RID*>(rec); }
inline RID& GetRecordID(void* rec) { assert(rec); return *reinterpret_cast<RID*>(rec); }

FID FindFid(const Field* flds, size_t count, PID pid);

bool SetRecordFieldData(void* field, const Field& info, size_t len, const void* val);

std::string GetRecordFieldString(const void* rec, const Field& info);
bool SetRecordFieldString(void* rec, const Field& info, const char* str);

void ClearRecordField(void* rec, const Field& info);

} // namespace memdb
