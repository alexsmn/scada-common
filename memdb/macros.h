#pragma once

#include "memdb/descriptor.h"
#include "memdb/field.h"

#define SIZEOF_MEMBER(cls, member) sizeof(((cls*)0)->member)

#define BEGIN_FIELD_MAP(cls, version) \
  typedef cls _class; \
  enum { VERSION = version }; \
  static const memdb::Descriptor& GetDescriptor() \
  { \
    static const memdb::Field fields[] = {

#define DEFINE_FIELD_EX(pid, name, mem, type, look_tid, look_val, look_dis) \
  { static_cast<uint8_t>(pid), static_cast<uint8_t>(cfg::PackedPropertyId::pid), \
    offsetof(_class, mem), SIZEOF_MEMBER(_class, mem), memdb::type, 0 },

#define DEFINE_FIELD_V(pid, name, mem, type) \
  { static_cast<uint8_t>(pid), static_cast<uint8_t>(cfg::PackedPropertyId::pid), \
    offsetof(_class, mem), SIZEOF_MEMBER(_class, mem), memdb::type, memdb::FIELD_VIRT },

#define DEFINE_FIELD_NS(pid, name, mem, type) \
  { static_cast<uint8_t>(pid), static_cast<uint8_t>(cfg::PackedPropertyId::pid), \
    offsetof(_class, mem), SIZEOF_MEMBER(_class, mem), memdb::type, memdb::FIELD_NS },

#define DEFINE_FIELD(pid, name, mem, type) \
  DEFINE_FIELD_EX(pid, name, mem, type, memdb::TID_INVAL, 0, 0)

#define DEFINE_FIELD_AUTO(pid, name, mem) \
  DEFINE_FIELD_EX(pid, name, mem, GetDBType(mem), memdb::TID_INVAL, 0, 0)

#define DEFINE_FIELD_LOOKUP(pid, name, mem, type, look_tid, look_val, look_dis) \
  DEFINE_FIELD_EX(pid, name, mem, type, look_tid, look_val, look_dis)

#define END_FIELD_MAP() \
    }; \
    static const memdb::Descriptor descriptor(fields, _countof(fields), sizeof(_class), VERSION); \
    return descriptor; \
  }
