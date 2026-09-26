#include "common/format.h"

#include "base/format.h"
#include "base/string_util.h"
#include "base/utf_convert.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "scada/variant.h"
#include <charconv>
#include <cmath>
#include <cstdio>

#include <algorithm>
#include <cstring>
#include <optional>

namespace {

// Set once at startup by the UI layer and read thereafter; a function-local
// static keeps it out of static-global-init ordering.
FallbackLabelProvider& GetFallbackLabelProvider() {
  static FallbackLabelProvider provider = nullptr;
  return provider;
}

// The invariant form of each label: what renders with no UI layer.
std::u16string_view InvariantFallbackLabel(FallbackLabel label) {
  switch (label) {
    case FallbackLabel::kDefaultClose:
      return u"1";
    case FallbackLabel::kDefaultOpen:
      return u"0";
    case FallbackLabel::kEmptyDisplayName:
    case FallbackLabel::kUnknownDisplayName:
      return u"#NAME?";
  }
  return {};
}

std::u16string GetFallbackLabel(FallbackLabel label) {
  if (const FallbackLabelProvider provider = GetFallbackLabelProvider())
    return provider(label);
  return std::u16string{InvariantFallbackLabel(label)};
}

}  // namespace

void SetFallbackLabelProvider(FallbackLabelProvider provider) {
  GetFallbackLabelProvider() = provider;
}

std::u16string DefaultCloseLabel() {
  return GetFallbackLabel(FallbackLabel::kDefaultClose);
}

std::u16string DefaultOpenLabel() {
  return GetFallbackLabel(FallbackLabel::kDefaultOpen);
}

std::u16string EmptyDisplayName() {
  return GetFallbackLabel(FallbackLabel::kEmptyDisplayName);
}

std::u16string UnknownDisplayName() {
  return GetFallbackLabel(FallbackLabel::kUnknownDisplayName);
}

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
  int n = std::snprintf(buffer, sizeof(buffer), "%.*f", static_cast<int>(rlen),
                        val);
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

namespace {

// Recognizes the spelled-out boolean labels a BOOL round-trips through.
//
// This accepts more than `Variant::TrueLabel()`/`FalseLabel()` produce, and has
// to. A configuration export writes the *localized* label (see
// `FormatHelperT<LocalizedText, bool>`), so which words land in the file
// depends on who wrote it: a client with the Russian catalog installed writes
// "Да", a server — which installs no `BooleanTextProvider` — writes the
// invariant "true" (it wrote "Yes" before core stopped carrying the English
// word), and every file exported before the labels were translatable carries
// the Russian unconditionally. All of them have to import anywhere. The
// alternatives here are therefore wire data, not UI text, which is why the
// Russian stays as a literal.
std::optional<bool> ParseBoolLabel(std::u16string_view str) {
  static constexpr std::u16string_view kTrue[] = {u"true", u"Yes", u"Да"};
  static constexpr std::u16string_view kFalse[] = {u"false", u"No", u"Нет"};

  if (IEqualsAscii(str, scada::Variant::FalseLabel()))
    return false;
  if (IEqualsAscii(str, scada::Variant::TrueLabel()))
    return true;
  for (const std::u16string_view alternative : kFalse) {
    if (IEqualsAscii(str, alternative))
      return false;
  }
  for (const std::u16string_view alternative : kTrue) {
    if (IEqualsAscii(str, alternative))
      return true;
  }
  return std::nullopt;
}

}  // namespace

bool StringToValue(std::u16string_view str,
                   scada::Variant::Type data_type,
                   scada::Variant& value) {
  if (data_type == scada::Variant::Type::BOOL) {
    if (const std::optional<bool> parsed = ParseBoolLabel(str)) {
      value = *parsed;
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

  return bool_value ? DefaultCloseLabel() : DefaultOpenLabel();
}

scada::LocalizedText FormatTit(double double_value,
                               const TitFormatParams& params) {
  std::u16string text;

  text = UtfConvert<char16_t>(
      FormatFloat(double_value, params.display_format.c_str()));

  if (!params.engineering_units.empty()) {
    text += u' ';
    text += params.engineering_units.text;
  }

  return text;
}
