#pragma once

#include <cstddef>
#include <cstdint>
#include <cfloat>	// for FLT_MIN
#include <utility> // std::pair

namespace memdb {

// basic types
typedef uint8_t	DBBool;
typedef uint8_t	DBUInt8;
typedef uint16_t DBUInt16;
typedef uint32_t DBUInt32;
typedef double DBFloat;

// field types
enum DBType { DB_UINT8 = 0,
	            DB_UINT16 = 1,
	            DB_UINT32 = 2,
	            DB_STRING = 3,
	            DB_BOOL = 4,
	            DB_FLOAT = 6,
	            DB_TID = 7 };

// property id
typedef uint8_t PID;

// table id
typedef uint8_t TID;
const TID TID_INVAL	= 0;

// record id
typedef uint32_t RID;
const RID RID_INVAL	= 0;
const RID RID_FIRST = 1;

// field id
typedef uint8_t FID;
const FID FID_ID = 0;
const FID FID_INVAL = 0xff;

// user id
typedef RID UID;
const UID UID_INVAL	= RID_INVAL;

typedef int64_t RecordUID;

// null values
const DBFloat	NULL_FLOAT	= FLT_MIN;
const DBBool	NULL_BOOL	= 0xff;
const DBUInt8	NULL_UINT8	= 0xff;
const DBUInt16	NULL_UINT16	= 0xffff;
const DBUInt32	NULL_UINT32	= 0xffffffff;
const TID		NULL_TID	= TID_INVAL;

template<typename T> inline DBType GetDBType(T);
template<> inline DBType GetDBType(DBUInt8)					{ return DB_UINT8; }
template<> inline DBType GetDBType(DBUInt16)				{ return DB_UINT16; }
template<> inline DBType GetDBType(DBUInt32)				{ return DB_UINT32; }
template<> inline DBType GetDBType(const char*)				{ return DB_STRING; }
template<> inline DBType GetDBType(double)					{ return DB_FLOAT; }

template<typename T> inline T GetNullValue(T);
template<> inline DBUInt8 GetNullValue(DBUInt8)			{ return NULL_UINT8; }
template<> inline DBUInt16 GetNullValue(DBUInt16)			{ return NULL_UINT16; }
template<> inline DBUInt32 GetNullValue(DBUInt32)			{ return NULL_UINT32; }
template<> inline const char* GetNullValue(const char*)	{ return NULL; }
template<> inline DBFloat GetNullValue(DBFloat)			{ return NULL_FLOAT; }

template<typename T> inline bool IsNull(T value)	{
  return value == GetNullValue(value);
}

template<typename T, typename K>
inline K IfNull(T value, K value_if_null) {
	return IsNull(value) ? value_if_null : static_cast<K>(value);
}

template<>
inline bool IfNull(DBBool value, bool value_if_null) {
	return IsNull(value) ? value_if_null : (value != 0);
}

// conversions
inline bool FloatToBool(double value) { return value != 0.0; }
inline double BoolToFloat(bool value) { return value ? 1.0 : 0.0; }
inline DBBool DBFloatToBool(DBFloat value) {
  return IsNull(value) ? NULL_BOOL : FloatToBool(value);
}
inline DBFloat DBBoolToFloat(DBBool value) {
  return IsNull(value) ? NULL_FLOAT : BoolToFloat(value != 0);
}

typedef std::pair<TID, RID> TRID;

const TRID TRID_INVAL(TID_INVAL, RID_INVAL);

} // namespace memdb
