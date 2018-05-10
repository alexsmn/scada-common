#include "common/namespaces.h"

#include <cstring>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

// WARNING: These names are used as names for tables in the configuration DB and
// mustn't be modified.
const char* kNamespaceNames[NamespaceIndexes::END] = {
    "NS0",          "TS",           "TIT",           "MODBUS_DEVICES",
    "GROUP",        "USER",         "HISTORICAL_DB", "SCADA",
    "EXPR",         "SIM_ITEM",     "IEC_LINK",      "IEC_DEV",
    "MODBUS_PORTS", "FILE",         "TS_PARAMS",     "SERVER_PARAMS",
    "IEC_TRANSMIT", "IEC61850_DEV", "IEC61850_RCB",
};

const char* GetNamespaceName(scada::NamespaceIndex namespace_index) {
  if (namespace_index >= 0 && namespace_index < NamespaceIndexes::END)
    return kNamespaceNames[namespace_index];
  else
    return nullptr;
}

int FindNamespaceIndexByName(base::StringPiece name) {
  if (name.empty())
    return -1;

  int namespace_index = -1;
  if (name[0] == 'T' && base::StringToInt(name.substr(1), &namespace_index))
    return namespace_index;

  if (base::StringToInt(name, &namespace_index))
    return namespace_index;

  for (int i = 0; i != NamespaceIndexes::END; ++i) {
    if (base::EqualsCaseInsensitiveASCII(GetNamespaceName(i), name))
      return i;
  }

  return -1;
}
