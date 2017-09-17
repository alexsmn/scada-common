#pragma once

#include "base/time/time.h"
#include "core/attribute_ids.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/status.h"
#include "core/variant.h"
#include "core/extension_object.h"

#include <opcuapp/structs.h>
#include <opcuapp/types.h>
#include <opcuapp/vector.h>

scada::NodeClass ConvertNodeClass(opcua::NodeClass node_class);

scada::StatusCode ConvertStatusCode(opcua::StatusCode status_code);
opcua::StatusCode MakeStatusCode(scada::StatusCode status_code);

scada::Variant Convert(OpcUa_Variant&& source);
OpcUa_Variant MakeVariant(scada::Variant&& source);

scada::Qualifier MakeQualifier(opcua::StatusCode source);

scada::String Convert(const OpcUa_String& source);

base::Time Convert(OpcUa_DateTime source);
OpcUa_DateTime Convert(base::Time source);

scada::DataValue Convert(OpcUa_DataValue&& source);
OpcUa_DataValue MakeDataValue(scada::DataValue&& source);

OpcUa_NodeId Convert(const scada::NodeId& node_id);

scada::NodeId Convert(const OpcUa_NodeId& node_id);

scada::NodeId Convert(const OpcUa_ExpandedNodeId& node_id);

scada::AttributeId ConvertAttributeId(OpcUa_Int32 attribute_id);
OpcUa_Int32 Convert(scada::AttributeId attribute_id);

scada::ReadValueId Convert(const OpcUa_ReadValueId& source);
OpcUa_ReadValueId MakeUaReadValueId(const scada::ReadValueId& source);

OpcUa_BrowseDirection Convert(scada::BrowseDirection source);
scada::BrowseDirection Convert(OpcUa_BrowseDirection source);

scada::BrowseDescription Convert(const OpcUa_BrowseDescription& source);
OpcUa_BrowseDescription Convert(const scada::BrowseDescription& source);

scada::ReferenceDescription Convert(const OpcUa_ReferenceDescription& source);
OpcUa_ReferenceDescription Convert(const scada::ReferenceDescription& source);

scada::BrowseResult Convert(const OpcUa_BrowseResult& source);
OpcUa_BrowseResult Convert(const scada::BrowseResult& source);

scada::ExtensionObject Convert(OpcUa_ExtensionObject&& object);

scada::ByteString Convert(const OpcUa_ByteString& source);

scada::String Convert(const opcua::String& source);
scada::String Convert(const opcua::QualifiedName& source);
scada::LocalizedText Convert(const opcua::LocalizedText& source);

template<typename T, class It>
inline std::vector<T> ConvertVector(It first, It last) {
  std::vector<T> result(std::distance(first, last));
  std::transform(first, last, result.begin(),
      [](auto&& source) { return Convert(std::move(source)); });
  return result;
}

template<typename T, class Range>
inline std::vector<T> ConvertVector(Range&& range) {
  return ConvertVector<T>(std::begin(range), std::end(range));
}

template<typename T, typename S>
opcua::Vector<T> MakeVector(opcua::Span<S> values) {
  opcua::Vector<T> result(values.size());
  std::transform(values.begin(), values.end(), result.begin(),
      [](S& source) { return Convert(std::move(source)); });
  return result;
}