#include "common/format.h"

#include <algorithm>

#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/dmg_fp/dmg_fp.h"
#include "common/node_util.h"
#include "model/scada_node_ids.h"
#include "core/tvq.h"

void FmtAddMods(const NodeRef& node,
                scada::Qualifier qualifier,
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

  if (IsInstanceOf(node, id::DataItemType) &&
      node[id::DataItemType_Locked].value().get_or(false))
    mods += L'Б';

  if (qualifier.stale())
    mods += L'У';

  if (qualifier.limit() != scada::Qualifier::LIMIT_NORMAL)
    mods += L'В';

  if (mods.size() > 1) {
    /*if (flags&FORMAT_COLOR) {
      mods.insert(0, "{b");
      mods.append("}");
    } else*/

    mods += L']';
    mods += L' ';

    text.insert(0, mods);
  }
}

base::string16 FormatTsValue(const NodeRef& node,
                             const scada::Variant& value,
                             scada::Qualifier qualifier,
                             int flags) {
  base::string16 text;

  bool bool_value;
  if (value.get(bool_value)) {
    auto format = node.target(id::HasTsFormat);
    if (format) {
      auto pid =
          bool_value ? id::TsFormatType_CloseLabel : id::TsFormatType_OpenLabel;
      text = ToString16(format[pid].value().get_or(scada::LocalizedText()));
    } else {
      text = base::WideToUTF16(bool_value ? kDefaultCloseLabel : kDefaultOpenLabel);
    }
  }

  if ((flags & FORMAT_QUALITY) && qualifier.bad())
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

base::string16 FormatTitValue(const NodeRef& node,
                              const scada::Variant& value,
                              scada::Qualifier qualifier,
                              int flags) {
  base::string16 text;

  double double_value;
  if (value.get(double_value)) {
    auto format =
        node[id::AnalogItemType_DisplayFormat].value().get_or(scada::String());
    auto units =
        node[id::AnalogItemType_EngineeringUnits].value().get_or(scada::LocalizedText());
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

base::string16 FormatUnknownValue(const NodeRef& node,
                                  const scada::Variant& value,
                                  scada::Qualifier qualifier,
                                  int flags) {
  base::string16 text;

  if (value.get(text)) {
    if (flags & FORMAT_COLOR)
      EscapeColoredString(text);
  }

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += L'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

base::string16 FormatValue(const NodeRef& node,
                           const scada::Variant& value,
                           scada::Qualifier qualifier,
                           int flags) {
  if (IsInstanceOf(node, id::DiscreteItemType))
    return FormatTsValue(node, value, qualifier, flags);
  else if (IsInstanceOf(node, id::AnalogItemType))
    return FormatTitValue(node, value, qualifier, flags);
  else
    return FormatUnknownValue(node, value, qualifier, flags);
}
