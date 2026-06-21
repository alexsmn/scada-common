#pragma once

// Bidirectional converters between the SCADA `core` types (`scada::`) and the
// extracted opcuapp types (`opcua::scada::`). The two type universes are
// structurally identical mirrors, so most conversions are mechanical field
// copies. Notably, the std-alias types — `String` (std::string),
// `LocalizedText` (std::u16string), `ByteString` (std::vector<char>) and the
// numeric primitives — are the SAME std type on both sides and need no
// conversion at all; only the class types and `base::Time`-backed `DateTime`
// require real work.

#include "scada/data_value.h"
#include "scada/expanded_node_id.h"
#include "scada/extension_object.h"
#include "scada/node_id.h"
#include "scada/qualified_name.h"
#include "scada/qualifier.h"
#include "scada/status.h"
#include "scada/variant.h"

#include "opcua/scada/data_value.h"
#include "opcua/scada/expanded_node_id.h"
#include "opcua/scada/extension_object.h"
#include "opcua/scada/node_id.h"
#include "opcua/scada/qualified_name.h"
#include "opcua/scada/qualifier.h"
#include "opcua/scada/status.h"
#include "opcua/scada/variant.h"

#include <vector>

namespace opcua_bridge {

// --- enums (identical underlying values) --------------------------------
inline opcua::scada::StatusCode ToOpcua(scada::StatusCode c) {
  return static_cast<opcua::scada::StatusCode>(c);
}
inline scada::StatusCode ToScada(opcua::scada::StatusCode c) {
  return static_cast<scada::StatusCode>(c);
}

// --- Status -------------------------------------------------------------
inline opcua::scada::Status ToOpcua(scada::Status s) {
  return opcua::scada::Status::FromFullCode(s.full_code());
}
inline scada::Status ToScada(opcua::scada::Status s) {
  return scada::Status::FromFullCode(s.full_code());
}

// --- DateTime (base::Time vs opcua::base::Time) -------------------------
inline opcua::scada::DateTime ToOpcua(scada::DateTime t) {
  return opcua::base::Time::FromInternalValue(t.ToInternalValue());
}
inline scada::DateTime ToScada(opcua::scada::DateTime t) {
  return base::Time::FromInternalValue(t.ToInternalValue());
}

// --- Qualifier ----------------------------------------------------------
inline opcua::scada::Qualifier ToOpcua(scada::Qualifier q) {
  return opcua::scada::Qualifier{q.raw()};
}
inline scada::Qualifier ToScada(opcua::scada::Qualifier q) {
  return scada::Qualifier{q.raw()};
}

// --- class types --------------------------------------------------------
opcua::scada::NodeId ToOpcua(const scada::NodeId&);
scada::NodeId ToScada(const opcua::scada::NodeId&);

opcua::scada::ExpandedNodeId ToOpcua(const scada::ExpandedNodeId&);
scada::ExpandedNodeId ToScada(const opcua::scada::ExpandedNodeId&);

opcua::scada::QualifiedName ToOpcua(const scada::QualifiedName&);
scada::QualifiedName ToScada(const opcua::scada::QualifiedName&);

opcua::scada::ExtensionObject ToOpcua(const scada::ExtensionObject&);
scada::ExtensionObject ToScada(const opcua::scada::ExtensionObject&);

opcua::scada::Variant ToOpcua(const scada::Variant&);
scada::Variant ToScada(const opcua::scada::Variant&);

opcua::scada::DataValue ToOpcua(const scada::DataValue&);
scada::DataValue ToScada(const opcua::scada::DataValue&);

// The element-wise vector helpers (ToOpcuaVector / ToScadaVector) live in
// vector_conversion.h, which both .cpp files include AFTER all ToOpcua/ToScada
// overloads are declared, so ordinary lookup inside the template sees every
// element converter.

}  // namespace opcua_bridge
