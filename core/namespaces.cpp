#include "core/namespaces.h"

#include <string.h>

#include "base/strings/string_number_conversions.h"

const char* kNamespaceNames[static_cast<size_t>(NamespaceIndexes::END)] = {
  "OPCUA",
  "TS",
  "TIT",              
  "MODBUS_DEVICES",   
  "GROUP",
  "USER",
  "HISTORICAL_DB",
  "SUBS",
  "EXPR",
  "SIM_ITEM",
  "IEC_LINK",
  "IEC_DEV",
  "MODBUS_PORTS",
  "FILE",
  "TS_PARAMS",
  "SERVER_PARAMS",
  "IEC_TRANSMIT",
  "SCADA"
};

const char* GetNamespaceName(scada::NamespaceIndex namespace_index) {
  if (namespace_index >= 0 && namespace_index < static_cast<scada::NamespaceIndex>(NamespaceIndexes::END))
    return kNamespaceNames[namespace_index];
  else
    return nullptr;
}

int FindNamespaceIndexByName(const base::StringPiece& name) {
  if (name.empty())
    return -1;

  int namespace_index = -1;
  if (name[0] == 'T' && base::StringToInt(name.substr(1), &namespace_index))
    return namespace_index;

  if (base::StringToInt(name, &namespace_index))
    return namespace_index;

  for (int i = 0; i < static_cast<int>(NamespaceIndexes::END); ++i) {
    if (_strnicmp(GetNamespaceName(i), name.data(), name.size()) == 0)
      return i;
  }
  return -1;
}
