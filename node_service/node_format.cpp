#include "common/format.h"

#include <algorithm>

#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"

#include <format>
#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "scada/tvq.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_util.h"

void FmtAddMods(const NodeRef& node,
                scada::Qualifier qualifier,
                std::u16string& text,
                int flags) {
  std::u16string mods;
  mods.reserve(16);

  mods += u'[';

  if (qualifier.failed())
    mods += u'Н';
  else if (qualifier.misconfigured())
    mods += u'К';
  else if (qualifier.offline())
    mods += u'С';

  if (qualifier.manual())
    mods += u'Р';
  else if (qualifier.backup())
    mods += u'2';

  if (IsInstanceOf(node, data_items::id::DataItemType) &&
      node[data_items::id::DataItemType_Locked].value().get_or(false))
    mods += u'Б';

  if (qualifier.stale())
    mods += u'У';

  if (qualifier.limit() != scada::Qualifier::LIMIT_NORMAL)
    mods += u'В';

  if (mods.size() > 1) {
    /*if (flags&FORMAT_COLOR) {
      mods.insert(0, "{b");
      mods.append("}");
    } else*/

    mods += u']';
    mods += u' ';

    text.insert(0, mods);
  }
}

TsFormatParams GetTsFormatParams(const NodeRef& node) {
  auto format = node.target(data_items::id::HasTsFormat);
  if (!format) {
    return {};
  }

  return {.close_label =
              format[data_items::id::TsFormatType_CloseLabel].value().get_or(
                  scada::LocalizedText{}),
          .open_label =
              format[data_items::id::TsFormatType_OpenLabel].value().get_or(
                  scada::LocalizedText{})};
}

std::u16string FormatTsValue(const NodeRef& node,
                             const scada::Variant& value,
                             scada::Qualifier qualifier,
                             int flags) {
  std::u16string text;

  if (bool bool_value; value.get(bool_value)) {
    auto format_params = GetTsFormatParams(node);
    text = FormatTs(bool_value, format_params);
  }

  if ((flags & FORMAT_QUALITY) && qualifier.bad())
    text += u'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

TitFormatParams GetTitFormatParams(const NodeRef& node) {
  return {
      .display_format =
          node[data_items::id::AnalogItemType_DisplayFormat].value().get_or(
              scada::String()),
      .engineering_units =
          node[data_items::id::AnalogItemType_EngineeringUnits].value().get_or(
              scada::LocalizedText())};
}

std::u16string FormatTitValue(const NodeRef& node,
                              const scada::Variant& value,
                              scada::Qualifier qualifier,
                              int flags) {
  std::u16string text;

  double double_value;
  if (value.get(double_value)) {
    auto format_params = GetTitFormatParams(node);
    text = base::WideToUTF16(base::SysNativeMBToWide(
        FormatFloat(double_value, format_params.display_format.c_str())));
    if ((flags & FORMAT_UNITS) && !format_params.engineering_units.empty()) {
      text += u' ';
      if (flags & FORMAT_COLOR)
        text += base::WideToUTF16(base::SysNativeMBToWide(
            std::format("&color:{};", 0x7f7f7f)));
      text += format_params.engineering_units;
    }
  }

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += u'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

std::u16string FormatUnknownValue(const NodeRef& node,
                                  const scada::Variant& value,
                                  scada::Qualifier qualifier,
                                  int flags) {
  std::u16string text;

  if (value.get(text)) {
    if (flags & FORMAT_COLOR)
      EscapeColoredString(text);
  }

  if (qualifier.bad() && (flags & FORMAT_QUALITY))
    text += u'?';

  if (flags & FORMAT_STATUS)
    FmtAddMods(node, qualifier, text, flags);

  return text;
}

std::u16string FormatValue(const NodeRef& node,
                           const scada::Variant& value,
                           scada::Qualifier qualifier,
                           int flags) {
  if (IsInstanceOf(node, data_items::id::DiscreteItemType))
    return FormatTsValue(node, value, qualifier, flags);
  else if (IsInstanceOf(node, data_items::id::AnalogItemType))
    return FormatTitValue(node, value, qualifier, flags);
  else
    return FormatUnknownValue(node, value, qualifier, flags);
}
