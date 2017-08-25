#include "memdb/descriptor.h"

#include "base/bytemsg.h"
#include "memdb/field_value.h"

namespace memdb {

Descriptor::Descriptor(const Field* fields, size_t field_count,
                       size_t record_size, unsigned version)
    : num_fields_(field_count),
      fields_(fields),
      record_size_(record_size),
      version_(version) {
}

DBUInt32 Descriptor::GetUInt32(const void* rec, unsigned field) const {
  assert(field < num_fields_);
  const char* ptr = reinterpret_cast<const char*>(rec) +
    fields_[field].offset;
  switch (fields_[field].type) {
    case DB_BOOL:
      assert(fields_[field].size == sizeof(DBBool));
      return IfNull(*reinterpret_cast<const DBBool*>(ptr), NULL_UINT32);
    case DB_UINT8:
      assert(fields_[field].size == sizeof(DBUInt8));
      return IfNull(*reinterpret_cast<const DBUInt8*>(ptr), NULL_UINT32);
    case DB_UINT16:
      assert(fields_[field].size == sizeof(DBUInt16));
      return IfNull(*reinterpret_cast<const DBUInt16*>(ptr), NULL_UINT32);
    case DB_UINT32:
      assert(fields_[field].size == sizeof(DBUInt32));
      return *reinterpret_cast<const DBUInt32*>(ptr);
    default:
      assert(false);
      return 0;
  }
}

const char* Descriptor::GetString(const void* rec, unsigned field) const {
  assert(field < num_fields_);
  assert(fields_[field].type == DB_STRING);
  return reinterpret_cast<const char*>(
    reinterpret_cast<const uint8_t*>(rec) + fields_[field].offset);
}

bool Descriptor::SetUInt32(void* rec, unsigned field, DBUInt32 value) const {
  assert(field < num_fields_);

  char* ptr = reinterpret_cast<char*>(rec) + fields_[field].offset;
  switch (fields_[field].type) {
    case DB_UINT8:
      assert(fields_[field].size == sizeof(DBUInt8));
      if (value != NULL_UINT32 && (value & ~0xff) != value)
        return false;
      *reinterpret_cast<DBUInt8*>(ptr) = IfNull(value, NULL_UINT8);
      return true;

    case DB_UINT16:
      assert(fields_[field].size == sizeof(DBUInt16));
      if (value != NULL_UINT32 && (value & ~0xffff) != value)
        return false;
      *reinterpret_cast<DBUInt16*>(ptr) = IfNull(value, NULL_UINT16);
      return true;

    case DB_UINT32:
      assert(fields_[field].size == sizeof(DBUInt32));
      *reinterpret_cast<DBUInt32*>(ptr) = value;
      return true;

    default:
      return false;
  }
}

bool Descriptor::SetString(void* rec, unsigned field, const char* text) const {
  assert(field != FID_INVAL);
  assert(field < num_fields_);
  return SetRecordFieldString(rec, fields_[field], text);
}

bool Descriptor::SetValue(void* record, const FieldValue& value) const {
  assert(value.field != FID_INVAL);
  assert(value.field != FID_ID);
  assert(value.field < num_fields_);
    
  const Field* finfo = &fields_[value.field];
  
  if (finfo->type == DB_STRING) {
    const char* str = reinterpret_cast<const char*>(value.data);
    auto len = strlen(str);
    if (len >= finfo->size) {
      assert(false);
      return false;
    }

    char* ptr = reinterpret_cast<char*>(record) + finfo->offset;
    memcpy(ptr, str, len);
    ptr[len + 1] = '\0';
    
  } else {
    if (value.size != fields_[value.field].size) {
      assert(false);
      return false;
    }

    char* ptr = static_cast<char*>(const_cast<void*>(record)) + finfo->offset;
    memcpy(ptr, value.data, value.size);
  }

  return true;
}

size_t Descriptor::PackRecord(const void* rec, void* buffer, size_t size) const {
  ByteMessage buf(buffer, size);
  
  for (size_t i = 0; i < field_count(); ++i) {
    const Field& field = this->field(i);
    const char* ptr = reinterpret_cast<const char*>(rec) + field.offset;
    
    buf.WriteByte(field.packed_pid);
    size_t len = field.size;
    if (field.type == DB_STRING)
      len = strlen(ptr);
    assert(len == static_cast<uint16_t>(len));
    buf.WriteWord(static_cast<uint16_t>(len));
    buf.Write(ptr, len);
  }
  
  return buf.size;
}

bool Descriptor::UnpackRecord(void* rec, const void* buffer, size_t size) const {
  ByteMessage buf(const_cast<void*>(buffer), size, size);
  
  while (!buf.end()) {
    PID pid = buf.ReadByte();
    size_t len = buf.ReadWord();

    const Field* field = FindPackedField(pid);
    if (!field) {
      // Skip incorrect fields.
      buf.Read(NULL, len);
      continue;
    }
      
    if (field->type != DB_STRING && len != field->size) {
      // Skip incorrect fields.
      buf.Read(NULL, len);
      continue;
    }

    char* ptr = reinterpret_cast<char*>(rec) + field->offset;
    
    if (field->type == DB_STRING && len + 1 > field->size)
      len = field->size - 1;
    
    buf.Read(ptr, len);
    
    if (field->type == DB_STRING)
      ptr[len] = '\0';
  }

  return true;
}

size_t Descriptor::GetDifferentFieldValues(const void* record1,
                                           const void* record2,
                                           FieldValue values[]) const {
  size_t count = 0;

  for (size_t i = 0; i < field_count(); ++i) {
    const Field& field = this->field(i);
    
    bool is_updated;
    const char* ptr = static_cast<const char*>(record1) + field.offset;
    const char* nptr = static_cast<const char*>(record2) + field.offset;
    if (field.type == DB_STRING)
      is_updated = strcmp(ptr, nptr) != 0;
    else
      is_updated = memcmp(ptr, nptr, field.size) != 0;
    if (!is_updated)
      continue;

    FieldValue& value = values[count++];
    value.field = i;
    value.size = field.type == DB_STRING ? strlen(nptr) : field.size;
    value.data = nptr;
  }

  return count;
}

size_t Descriptor::GetAllFieldValues(const void* record,
                                     FieldValue values[]) const {
  for (unsigned i = 0; i < field_count(); ++i) {
    const Field& field = this->field(i);
    FieldValue& value = values[i];

    const char* ptr = static_cast<const char*>(record) + field.offset;
    value.field = i;
    value.data = ptr;
    value.size = field.type == DB_STRING ? strlen(ptr) : field.size;
  }

  return field_count();
}

FID Descriptor::FindFID(PID pid) const {
  for (FID i = 0; i < num_fields_; ++i)
    if (fields_[i].pid == pid)
      return i;
  return FID_INVAL;
}

const Field* Descriptor::FindField(PID pid) const {
  FID fid = FindFID(pid);
  return fid != FID_INVAL ? &fields_[fid] : NULL;
}

const Field* Descriptor::FindPackedField(PID pid) const {
  for (FID i = 0; i < num_fields_; ++i)
    if (fields_[i].packed_pid == pid)
      return &fields_[i];
  return NULL;
}

} // namespace memdb
