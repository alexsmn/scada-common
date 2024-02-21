#include "common/format.h"

#include "base/format.h"
#include "base/string_piece_util.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/dmg_fp/dmg_fp.h"
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
  base::ReplaceSubstringsAfterOffset(&str, 0, amp, amp);
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

  int decimal = 0;
  int sign = 0;
  //  char* s = fcvt(val, rlen, &decimal, &sign);
  char* e = nullptr;
  char* s = dmg_fp::dtoa(val, 3, rlen, &decimal, &sign, &e);
  int l = static_cast<int>(e - s);
  char buffer[64] = {0};
  size_t buffer_size = 0;

  if (sign)
    buffer[buffer_size++] = '-';

  if (decimal <= 0) {
    buffer[buffer_size++] = '0';

  } else {
    int n = (std::min)(decimal, l);
    memcpy(buffer + buffer_size, s, n);
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
    memcpy(buffer + buffer_size, s + decimal, l - decimal);
    buffer_size += l - decimal;
  }

  dmg_fp::freedtoa(s);

  return std::string(buffer, buffer_size);
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

  return StringToValue(base::UTF16ToUTF8(AsStringPiece(str)), data_type, value);
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

  text = base::ASCIIToUTF16(
      FormatFloat(double_value, params.display_format.c_str()));

  if (!params.engineering_units.empty()) {
    text += u' ';
    text += params.engineering_units;
  }

  return text;
}
