#include "base/format.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/dmg_fp/dmg_fp.h"

#include <memory>


std::string Format(double value) {
  char buffer[32];
  dmg_fp::g_fmt(buffer, value);
  return std::string(buffer);
}

base::string16 WideFormat(double value) {
  char buffer[32];
  dmg_fp::g_fmt(buffer, value);
  return base::string16(buffer, buffer + strlen(buffer));
}

std::string Format(float value) {
  return Format(static_cast<double>(value));
}

base::string16 WideFormat(float value) {
  return WideFormat(static_cast<double>(value));
}

std::string Format(int value) {
  return base::IntToString(value);
}

base::string16 WideFormat(int value) {
  return base::IntToString16(value);
}

std::string Format(unsigned int value) {
  return base::UintToString(value);
}

base::string16 WideFormat(unsigned int value) {
  return base::UintToString16(value);
}

std::string Format(uint64_t value) {
  return base::Uint64ToString(value);
}

std::string Format(int64_t value) {
  return base::Int64ToString(value);
}

base::string16 WideFormat(int64_t value) {
  return base::Int64ToString16(value);
}

base::string16 WideFormat(uint64_t value) {
  return base::Uint64ToString16(value);
}

std::string Format(bool value) {
  return Format(value ? 1 : 0);
}

base::string16 WideFormat(bool value) {
  return WideFormat(value ? 1 : 0);
}

std::string Format(unsigned char value) {
  return Format(static_cast<unsigned>(value));
}

base::string16 WideFormat(unsigned char value) {
  return WideFormat(static_cast<unsigned>(value));
}

std::string Format(unsigned short value) {
  return Format(static_cast<unsigned>(value));
}

base::string16 WideFormat(unsigned short value) {
  return WideFormat(static_cast<unsigned>(value));
}

std::string Format(base::StringPiece value) {
  return value.as_string();
}

base::string16 WideFormat(base::StringPiece value) {
  return base::WideToUTF16(base::SysNativeMBToWide(value.as_string()));
}

std::string Format(base::StringPiece16 value) {
  return base::SysWideToNativeMB(base::UTF16ToWide(value.as_string()));
}

base::string16 WideFormat(base::StringPiece16 value) {
  return value.as_string();
}

template<>
bool Parse<int>(const base::StringPiece& str, int& value) {
  return base::StringToInt(str, &value);
}

template<>
bool Parse<int>(const base::StringPiece16& str, int& value) {
  return base::StringToInt(str, &value);
}

template<>
bool Parse<unsigned>(const base::StringPiece& str, unsigned& value) {
  return base::StringToUint(str, &value);
}

template<>
bool Parse<unsigned>(const base::StringPiece16& str, unsigned& value) {
  return base::StringToUint(str, &value);
}

template<>
bool Parse<unsigned char>(const base::StringPiece& str, unsigned char& value) {
  unsigned tmp;
  if (!Parse(str, tmp))
    return false;
  value = static_cast<unsigned char>(tmp);
  return value == tmp;
}

template<>
bool Parse<unsigned char>(const base::StringPiece16& str, unsigned char& value) {
  unsigned tmp;
  if (!Parse(str, tmp))
    return false;
  value = static_cast<unsigned char>(tmp);
  return value == tmp;
}

template<>
bool Parse<unsigned short>(const base::StringPiece& str, unsigned short& value) {
  unsigned tmp;
  if (!base::StringToUint(str, &tmp))
    return false;
  value = static_cast<unsigned short>(tmp);
  return value == tmp;
}

template<>
bool Parse<unsigned short>(const base::StringPiece16& str, unsigned short& value) {
  unsigned tmp;
  if (!base::StringToUint(str, &tmp))
    return false;
  value = static_cast<unsigned short>(tmp);
  return value == tmp;
}

template<>
bool Parse(const base::StringPiece& str, uint64_t& value) {
  return base::StringToUint64(str, &value);
}

template<>
bool Parse<uint64_t>(const base::StringPiece16& str, uint64_t& value) {
  return base::StringToUint64(str, &value);
}

template<>
bool Parse<double>(const base::StringPiece& str, double& value) {
  return base::StringToDouble(str.as_string(), &value);
}
template<>
bool Parse<double>(const base::StringPiece16& str, double& value) {
  // TODO: Use ICU for conversion.
  return base::StringToDouble(base::UTF16ToASCII(str.as_string()), &value);
}

template<>
bool Parse<bool>(const base::StringPiece& str, bool& value) {
  int tmp;
  if (!Parse(str, tmp))
    return false;
  value = tmp != 0;
  return true;
}
template<>
bool Parse<bool>(const base::StringPiece16& str, bool& value) {
  int tmp;
  if (!Parse(str, tmp))
    return false;
  value = tmp != 0;
  return true;
}

template<>
bool Parse<std::string>(const base::StringPiece& str, std::string& value) {
  value = str.as_string();
  return true;
}
template<>
bool Parse<base::string16>(const base::StringPiece16& str, base::string16& value) {
  value = str.as_string();
  return true;
}

std::string FormatHexBuffer(const void* buf, size_t len) {
  assert(buf);
  if (!len)
    return std::string();

  return base::HexEncode(buf, len);
}
