#include "node_format.h"

#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"

namespace scada {

void FmtAddMods(bool locked,
                Qualifier qualifier,
                std::wstring& text,
                int flags) {
  std::wstring mods;
  mods.reserve(16);

  mods += L'[';

  if (qualifier.failed())
    mods += L'Н';
  else if (qualifier.misconfigured())
    mods += L'К';
  else if (qualifier.offline())
    mods += L'С';

  if (qualifier.manual())
    mods += L'Р';
  else if (qualifier.backup())
    mods += L'2';

  if (locked)
    mods += L'Б';

  if (qualifier.stale())
    mods += L'У';

  if (qualifier.limit() != Qualifier::LIMIT_NORMAL)
    mods += L'В';

  if (mods.size() > 1) {
    mods += L']';
    mods += L' ';
    text.insert(0, mods);
  }
}

std::wstring FormatDiscreteValue(const LocalizedText& open_label,
                                 const LocalizedText& close_label,
                                 bool locked,
                                 const Variant& value,
                                 Qualifier qualifier,
                                 int flags) {
  std::wstring text;

  bool bool_value;
  if (value.get(bool_value)) {
    text = bool_value ? close_label : open_label;
  }

  if ((flags & FORMAT_QUALITY) && qualifier.bad())
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(locked, qualifier, text, flags);

  return text;
}

std::wstring FormatAnalogValue(const String& display_format,
                               const LocalizedText& eu_units,
                               bool locked,
                               const Variant& value,
                               Qualifier qualifier,
                               int flags) {
  std::wstring text;

  double double_value;
  if (value.get(double_value)) {
    text = base::WideToUTF16(base::SysNativeMBToWide(
        FormatFloat(double_value, display_format.c_str())));
    if ((flags & FORMAT_UNITS) && !eu_units.empty()) {
      text += L' ';
      if (flags & FORMAT_COLOR)
        text += base::WideToUTF16(base::SysNativeMBToWide(
            base::StringPrintf("&color:%d;", 0x7f7f7f)));
      text += eu_units;
    }
  }

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(locked, qualifier, text, flags);

  return text;
}

std::wstring FormatUnknownValue(bool locked,
                                const Variant& value,
                                Qualifier qualifier,
                                int flags) {
  std::wstring text;

  value.get(text);

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(locked, qualifier, text, flags);

  return text;
}

std::wstring FormatValue(const Node& node,
                         const Variant& value,
                         Qualifier qualifier,
                         int flags) {
  const auto locked =
      GetPropertyValue(node, data_items::id::DataItemType_Locked).get_or(false);

  if (IsInstanceOf(&node, data_items::id::DiscreteItemType)) {
    LocalizedText open_label, close_label;
    if (auto* format = GetTsFormat(node)) {
      open_label =
          GetPropertyValue(*format, data_items::id::TsFormatType_OpenLabel)
              .get_or(LocalizedText());
      close_label =
          GetPropertyValue(*format, data_items::id::TsFormatType_CloseLabel)
              .get_or(LocalizedText());
    } else {
      open_label = base::WideToUTF16(kDefaultOpenLabel);
      close_label = base::WideToUTF16(kDefaultCloseLabel);
    }

    return FormatDiscreteValue(open_label, close_label, locked, value,
                               qualifier, flags);

  } else if (IsInstanceOf(&node, data_items::id::AnalogItemType)) {
    auto display_format =
        GetPropertyValue(node, data_items::id::AnalogItemType_DisplayFormat)
            .get_or(String());
    auto eu_units =
        GetPropertyValue(node, data_items::id::AnalogItemType_EngineeringUnits)
            .get_or(LocalizedText());
    return FormatAnalogValue(display_format, eu_units, locked, value, qualifier,
                             flags);

  } else {
    return FormatUnknownValue(locked, value, qualifier, flags);
  }
}

}  // namespace scada
