#include "memdb/field.h"

#include "base/format.h"

namespace memdb {

namespace {

bool EqualNoCase(const char* a, const char* b) {
  auto la = strlen(a);
  auto lb = strlen(b);
  if (la != lb)
    return false;
  return std::equal(a, a + la, b, [](char a, char b) { return tolower(a) == tolower(b); });
}

} // namespace

const char* kTrueString = "Да";
const char* kFalseString = "Нет";

FID FindFid(const Field* flds, size_t count, PID pid) {
  assert(count >= 0);
  for (FID i = 0; i < count; ++i)
    if (flds[i].pid == pid)
      return i;
  return FID_INVAL;
}

bool SetRecordFieldData(void* rec, const Field& info, size_t len, const void* val) {
  assert(rec);
  assert(val);

  char* ptr = (char*)rec + info.offset;

  if (info.type == DB_STRING) {
    if (len >= info.size)
      return false;
    memcpy(ptr, val, len);
    ptr[len + 1] = '\0';

  } else {
    if (len != info.size)
      return false;
    memcpy(ptr, val, len);
  }

  return true;
}

std::string GetRecordFieldString(const void* val, const Field& info) {
  assert(val);

  std::string str;

  val = static_cast<const uint8_t*>(val) + info.offset;

  switch (info.type) {
    case DB_UINT8:
      if (*(DBUInt8*)val != NULL_UINT8)
        str = Format(*(DBUInt8*)val);
      break;
    case DB_UINT16:
      if (*(DBUInt16*)val != NULL_UINT16)
        str = Format(*(DBUInt16*)val);
      break;
    case DB_UINT32:
      if (*(DBUInt32*)val != NULL_UINT32)
        str = Format(*(DBUInt32*)val);
      break;
    case DB_FLOAT:
      if (*(DBFloat*)val != NULL_FLOAT)
        str = Format(*(DBFloat*)val);
      break;
    case DB_STRING:
      if (*(char*)val)
        str = (char*)val;
      break;
    case DB_BOOL:
      if (*(DBBool*)val != NULL_BOOL)
        str = *(DBBool*)val ? kTrueString : kFalseString;
      break;
  }

  return str;
}

bool SetFieldString(void* val, const Field& info, const char* str) {
  switch (info.type) {
    case DB_UINT8:
      assert(info.size == sizeof(DBUInt8));
      if (!Parse(str, *static_cast<DBUInt8*>(val)))
        return false;
      break;

    case DB_UINT16:
      assert(info.size == sizeof(DBUInt16));
      if (!Parse(str, *static_cast<DBUInt16*>(val)))
        return false;
      break;

    case DB_UINT32:
      assert(info.size == sizeof(DBUInt32));
      if (!Parse(str, *static_cast<DBUInt32*>(val)))
        return false;
      break;

    case DB_FLOAT:
      assert(info.size == sizeof(DBFloat));
      if (!Parse(str, *static_cast<DBFloat*>(val)))
        return false;
      break;

    case DB_STRING: {
      auto len = strlen(str);
      if (len >= info.size)
        return false;
      memcpy(val, str, len);
      reinterpret_cast<char*>(val)[len + 1] = '\0';
      break;
    }

    case DB_BOOL:
      assert(info.size == sizeof(DBBool));
      if (EqualNoCase(str, "Yes") || EqualNoCase(str, kTrueString))
        *static_cast<DBBool*>(val) = 1;
      else if (EqualNoCase(str, "No") || EqualNoCase(str, kFalseString))
        *static_cast<DBBool*>(val) = 0;
      else
        return false;
      break;
      
    default:
      return false;
  }

  return true;
}

void ClearField(void* val, const Field& info) {
  switch (info.type) {
    case DB_UINT8:
      assert(info.size == sizeof(DBUInt8));
      *static_cast<DBUInt8*>(val) = NULL_UINT8;
      break;
    case DB_UINT16:
      assert(info.size == sizeof(DBUInt16));
      *static_cast<DBUInt16*>(val) = NULL_UINT16;
      break;
    case DB_UINT32:
      assert(info.size == sizeof(DBUInt32));
      *static_cast<DBUInt32*>(val) = NULL_UINT32;
      break;
    case DB_FLOAT:
      assert(info.size == sizeof(DBFloat));
      *static_cast<DBFloat*>(val) = NULL_FLOAT;
      break;
    case DB_STRING:
      assert(info.size != 0);
      *static_cast<char*>(val) = '\0';
      break;
    case DB_BOOL:
      assert(info.size == sizeof(DBBool));
      *static_cast<DBBool*>(val) = NULL_BOOL;
      break;
    default:
      assert(false);
      break;
  }
}

bool SetRecordFieldString(void* rec, const Field& field, const char* str) {
  return SetFieldString((uint8_t*)rec + field.offset, field, str);
}

void ClearRecordField(void* rec, const Field& field) {
  ClearField((uint8_t*)rec + field.offset, field);
}

} // namespace memdb
