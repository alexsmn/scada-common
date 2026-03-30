#include "common/format.h"

#include "base/format.h"
#include "base/string_util.h"
#include "base/utf_convert.h"
#include <charconv>
#include <cmath>
#include <cstdio>
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "scada/variant.h"

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>

const char16_t kDefaultCloseLabel[] = u"Вкл";
const char16_t kDefaultOpenLabel[] = u"Откл";

const char16_t kEmptyDisplayName[] = u"#ИМЯ?";
const char16_t kUnknownDisplayName[] = u"#ИМЯ?";

void EscapeColoredString(std::u16string& str) {
  static const char16_t amp[] = u"&";
  ReplaceSubstringsAfterOffset(&str, 0, amp, amp);
}

std::string FormatFloat(double val, const char* fmt) {
  size_t flen = strlen(fmt);
  size_t llen;  // left part len (before dot)
  size_t rlen;  // right part len (after dot)

  const char* left = fmt;

  while (*left == '#')
    left++;

  const char* dot = strchr(left, '.');
  if (dot) {
    llen = dot - left;
    rlen = flen - llen - 1;
  } else {
    llen = flen;
    rlen = 0;
  }

  char buffer[64];
  int n = std::snprintf(buffer, sizeof(buffer), "%.*f",
                        static_cast<int>(rlen), val);
  if (n < 0 || n >= static_cast<int>(sizeof(buffer)))
    return {};

  // Strip trailing zeros for '#' format characters.
  // E.g., format "0.####" with value 10.0 → "10" instead of "10.0000".
  if (dot && rlen > 0) {
    size_t optional_digits = 0;
    for (const char* p = fmt + flen - 1; p > dot && *p == '#'; --p)
      ++optional_digits;

    if (optional_digits > 0) {
      size_t end = n;
      size_t min_decimals = rlen - optional_digits;
      size_t dot_pos = end;
      for (size_t i = 0; i < end; ++i) {
        if (buffer[i] == '.') {
          dot_pos = i;
          break;
        }
      }
      size_t decimals = end - dot_pos - 1;
      while (decimals > min_decimals && buffer[end - 1] == '0') {
        --end;
        --decimals;
      }
      if (decimals == 0 && end > 0 && buffer[end - 1] == '.')
        --end;
      n = static_cast<int>(end);
    }
  }

  return std::string(buffer, n);
}

template <class T, class String>
inline bool StringToValueHelper(String str, scada::Variant& value) {
  T v;
  if (!Parse(str, v))
    return false;
  value = v;
  return true;
}

bool StringToValue(std::string_view str,
                   scada::Variant::Type data_type,
                   scada::Variant& value) {
  if (data_type == scada::Variant::Type::COUNT) {
    return false;
  }

  if (str.empty()) {
    value = {};
    return true;
  }

  // TODO: Extract `scada::Variant::Visit`.
  switch (data_type) {
    case scada::Variant::Type::BOOL:
      return StringToValueHelper<bool>(str, value);

    case scada::Variant::Type::DOUBLE:
      return StringToValueHelper<scada::Double>(str, value);

    case scada::Variant::Type::INT8:
      return StringToValueHelper<scada::Int8>(str, value);

    case scada::Variant::Type::UINT8:
      return StringToValueHelper<scada::UInt8>(str, value);

    case scada::Variant::Type::INT16:
      return StringToValueHelper<scada::Int16>(str, value);

    case scada::Variant::Type::UINT16:
      return StringToValueHelper<scada::UInt16>(str, value);

    case scada::Variant::Type::INT32:
      return StringToValueHelper<scada::Int32>(str, value);

    case scada::Variant::Type::UINT32:
      return StringToValueHelper<scada::UInt32>(str, value);

    case scada::Variant::Type::INT64:
      return StringToValueHelper<scada::Int64>(str, value);

    case scada::Variant::Type::UINT64:
      return StringToValueHelper<scada::UInt64>(str, value);

    case scada::Variant::Type::STRING:
      value = std::string{str};
      return true;

    case scada::Variant::Type::NODE_ID: {
      auto node_id = NodeIdFromScadaString(str);
      if (node_id.is_null()) {
        return false;
      }
      value = std::move(node_id);
      return true;
    }

    default:
      return false;
  }
}

bool StringToValue(std::u16string_view str,
                   scada::Variant::Type data_type,
                   scada::Variant& value) {
  if (data_type == scada::Variant::Type::BOOL) {
    if (boost::algorithm::iequals(str, scada::Variant::kFalseString)) {
      value = false;
      return true;
    } else if (boost::algorithm::iequals(str, scada::Variant::kTrueString)) {
      value = true;
      return true;
    }

  } else if (data_type == scada::Variant::Type::LOCALIZED_TEXT) {
    value = scada::ToLocalizedText(str);
    return true;
  }

  return StringToValue(UtfConvert<char>(str), data_type, value);
}

scada::LocalizedText FormatTs(bool bool_value, const TsFormatParams& params) {
  const auto& label = bool_value ? params.close_label : params.open_label;
  if (!label.empty()) {
    return label;
  }

  return bool_value ? kDefaultCloseLabel : kDefaultOpenLabel;
}

scada::LocalizedText FormatTit(double double_value,
                               const TitFormatParams& params) {
  std::u16string text;

  text = UtfConvert<char16_t>(
      FormatFloat(double_value, params.display_format.c_str()));

  if (!params.engineering_units.empty()) {
    text += u' ';
    text += params.engineering_units;
  }

  return text;
}
