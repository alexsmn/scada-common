#include "common/format.h"

#include <algorithm>

#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/third_party/dmg_fp/dmg_fp.h"
#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "core/variant.h"

const wchar_t* kEmptyDisplayName = L"#ИМЯ?";
const wchar_t* kUnknownDisplayName = L"#ИМЯ?";

const wchar_t* kDefaultCloseLabel = L"Вкл";
const wchar_t* kDefaultOpenLabel = L"Откл";

void EscapeColoredString(base::string16& str) {
  base::ReplaceSubstringsAfterOffset(&str, 0, L"&", L"&;");
}

std::string FormatFloat(double val, const char* fmt) {
  size_t flen = strlen(fmt);
  size_t llen;  // left part len (before dot)
  size_t rlen;  // right part len (after dot)

  const char* left = fmt;
  const char* right = nullptr;

  while (*left == '#')
    left++;

  const char* dot = strchr(left, '.');
  if (dot) {
    llen = dot - left;
    rlen = flen - llen - 1;
    right = dot + 1;
  } else {
    llen = flen;
    rlen = 0;
  }

  int decimal = 0;
  int sign = 0;
  //  char* s = fcvt(val, rlen, &decimal, &sign);
  char* e = nullptr;
  char* s = dmg_fp::dtoa(val, 3, rlen, &decimal, &sign, &e);
  int l = e - s;
  char buffer[64] = {0};
  size_t buffer_size = 0;

  if (sign)
    buffer[buffer_size++] = '-';

  if (decimal <= 0) {
    buffer[buffer_size++] = '0';

  } else {
    int n = (std::min)(decimal, l);
    memcpy_s(buffer + buffer_size, _countof(buffer) - buffer_size, s, n);
    buffer_size += n;

    if (l < decimal) {
      memset(buffer + buffer_size, '0', decimal - l);
      buffer_size += decimal - l;
    }
  }

  if (l > decimal) {
    buffer[buffer_size++] = '.';
    if (decimal < 0) {
      memset(buffer + buffer_size, '0', -decimal);
      buffer_size += -decimal;
      decimal = 0;
    }
    memcpy_s(buffer + buffer_size, _countof(buffer) - buffer_size, s + decimal,
             l - decimal);
    buffer_size += l - decimal;
  }

  dmg_fp::freedtoa(s);

  return std::string(buffer, buffer_size);
}

template <class T, class String>
bool StringToValueHelper(String str, scada::Variant& value) {
  T v;
  if (!Parse(str, v))
    return false;
  value = v;
  return true;
}

bool StringToValue(base::StringPiece str,
                   const scada::NodeId& data_type_id,
                   scada::Variant& value) {
  assert(!data_type_id.is_null());

  if (str.empty()) {
    value = scada::Variant();
    return true;
  }

  if (data_type_id == scada::id::Boolean) {
    return StringToValueHelper<bool>(str, value);

  } else if (data_type_id == scada::id::Double) {
    return StringToValueHelper<double>(str, value);

  } else if (data_type_id == scada::id::Int32) {
    return StringToValueHelper<int>(str, value);

  } else if (data_type_id == scada::id::String) {
    value = str.as_string();
    return true;

  } else if (data_type_id == scada::id::NodeId) {
    auto node_id = NodeIdFromScadaString(str);
    if (node_id.is_null())
      return false;
    value = node_id;
    return true;

  } else
    return false;
}

bool StringToValue(base::StringPiece16 str,
                   const scada::NodeId& data_type_id,
                   scada::Variant& value) {
  if (data_type_id == scada::id::Boolean) {
    if (IsEqualNoCase(str, scada::Variant::kFalseString)) {
      value = false;
      return true;
    } else if (IsEqualNoCase(str, scada::Variant::kTrueString)) {
      value = true;
      return true;
    }

  } else if (data_type_id == scada::id::LocalizedText) {
    value = scada::ToLocalizedText(str.as_string());
    return true;
  }

  return StringToValue(base::SysWideToNativeMB(str.as_string()), data_type_id,
                       value);
}
