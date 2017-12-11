#pragma once

#include "core/basic_types.h"
#include "base/strings/string_piece.h"

namespace NamespaceIndexes {

const scada::NamespaceIndex TS = 1;
const scada::NamespaceIndex TIT = 2;
const scada::NamespaceIndex MODBUS_DEVICES = 3;
const scada::NamespaceIndex GROUP = 4;
const scada::NamespaceIndex USER = 5;
const scada::NamespaceIndex HISTORICAL_DB = 6;
const scada::NamespaceIndex SCADA = 7;
const scada::NamespaceIndex SIM_ITEM = 9;
const scada::NamespaceIndex IEC_LINK = 10;
const scada::NamespaceIndex IEC_DEV = 11;
const scada::NamespaceIndex MODBUS_PORTS = 12;
const scada::NamespaceIndex TS_FORMAT = 14;
const scada::NamespaceIndex SERVER_PARAMS = 15;
const scada::NamespaceIndex IEC_TRANSMIT = 16;

const scada::NamespaceIndex END = 18;
const scada::NamespaceIndex BEGIN = TS;
const scada::NamespaceIndex LAST = END;

} // namespace NamespaceIndexes

const char* GetNamespaceName(scada::NamespaceIndex namespace_index);
int FindNamespaceIndexByName(const base::StringPiece& name);
