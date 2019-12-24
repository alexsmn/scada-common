#include "node_format.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "model/scada_node_ids.h"

namespace scada {

void FmtAddMods(const Node& node,
                Qualifier qualifier,
                base::string16& text,
                int flags) {
  base::string16 mods;
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

  if (GetPropertyValue(node, ::id::DataItemType_Locked).get_or(false))
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

base::string16 FormatTsValue(const Node& node,
                             const Variant& value,
                             Qualifier qualifier,
                             int flags) {
  base::string16 text;

  bool bool_value;
  if (value.get(bool_value)) {
    auto* format = GetTsFormat(node);
    if (format) {
      auto pid = bool_value ? ::id::TsFormatType_CloseLabel
                            : ::id::TsFormatType_OpenLabel;
      text = GetPropertyValue(*format, pid).get_or(scada::LocalizedText());
    } else {
      text = bool_value ? base::WideToUTF16(kDefaultCloseLabel)
                        : base::WideToUTF16(kDefaultOpenLabel);
    }
  }

  if ((flags & FORMAT_QUALITY) && qualifier.bad())
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

base::string16 FormatTitValue(const Node& node,
                              const Variant& value,
                              Qualifier qualifier,
                              int flags) {
  base::string16 text;

  double double_value;
  if (value.get(double_value)) {
    auto format = GetPropertyValue(node, ::id::AnalogItemType_DisplayFormat)
                      .get_or(scada::String());
    auto units = GetPropertyValue(node, ::id::AnalogItemType_EngineeringUnits)
                     .get_or(scada::LocalizedText());
    text = base::WideToUTF16(base::SysNativeMBToWide(FormatFloat(double_value, format.c_str())));
    if ((flags & FORMAT_UNITS) && !units.empty()) {
      text += L' ';
      if (flags & FORMAT_COLOR)
        text += base::WideToUTF16(base::SysNativeMBToWide(base::StringPrintf("&color:%d;", 0x7f7f7f)));
      text += units;
    }
  }

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

base::string16 FormatUnknownValue(const Node& node,
                                  const Variant& value,
                                  Qualifier qualifier,
                                  int flags) {
  base::string16 text;

  value.get(text);

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

base::string16 FormatValue(const Node& node,
                           const Variant& value,
                           Qualifier qualifier,
                           int flags) {
  if (IsInstanceOf(&node, ::id::DiscreteItemType))
    return FormatTsValue(node, value, qualifier, flags);
  else if (IsInstanceOf(&node, ::id::AnalogItemType))
    return FormatTitValue(node, value, qualifier, flags);
  else
    return FormatUnknownValue(node, value, qualifier, flags);
}

}  // namespace scada
