#pragma once

#include "base/time/time.h"
#include "core/attribute_ids.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/status.h"
#include "core/variant.h"
#include "core/extension_object.h"
#include "core/expanded_node_id.h"

#include <opcuapp/structs.h>
#include <opcuapp/types.h>
#include <opcuapp/vector.h>

scada::NodeClass ConvertNodeClass(opcua::NodeClass node_class);
opcua::NodeClass Convert(scada::NodeClass node_class);

scada::StatusCode ConvertStatusCode(opcua::StatusCode status_code);
opcua::StatusCode MakeStatusCode(scada::StatusCode status_code);

scada::Variant Convert(OpcUa_Variant&& source);
OpcUa_Variant MakeVariant(scada::Variant&& source);

scada::Qualifier MakeQualifier(opcua::StatusCode source);

scada::String Convert(const OpcUa_String& source);

scada::DateTime Convert(OpcUa_DateTime source);
OpcUa_DateTime Convert(scada::DateTime source);

scada::DataValue Convert(OpcUa_DataValue&& source);
OpcUa_DataValue MakeDataValue(scada::DataValue&& source);

void Convert(const scada::NodeId& source, OpcUa_NodeId& target);

scada::NodeId Convert(const OpcUa_NodeId& node_id);

scada::ExpandedNodeId Convert(const OpcUa_ExpandedNodeId& node_id);
void Convert(const scada::ExpandedNodeId& source, OpcUa_ExpandedNodeId& target);

scada::AttributeId ConvertAttributeId(OpcUa_Int32 attribute_id);
OpcUa_Int32 Convert(scada::AttributeId attribute_id);

scada::ReadValueId Convert(const OpcUa_ReadValueId& source);
OpcUa_ReadValueId MakeUaReadValueId(const scada::ReadValueId& source);

OpcUa_BrowseDirection Convert(scada::BrowseDirection source);
scada::BrowseDirection Convert(OpcUa_BrowseDirection source);

scada::BrowseDescription Convert(const OpcUa_BrowseDescription& source);
OpcUa_BrowseDescription Convert(const scada::BrowseDescription& source);

scada::ReferenceDescription Convert(const OpcUa_ReferenceDescription& source);
void Convert(const scada::ReferenceDescription& source, OpcUa_ReferenceDescription& target);

scada::BrowseResult Convert(const OpcUa_BrowseResult& source);
void Convert(const scada::BrowseResult& source, OpcUa_BrowseResult& target);

scada::ExtensionObject Convert(OpcUa_ExtensionObject&& object);
void Convert(scada::ExtensionObject&& source, OpcUa_ExtensionObject& target);

scada::ByteString Convert(const OpcUa_ByteString& source);

scada::String Convert(const opcua::String& source);
void Convert(const scada::String& source, OpcUa_String& target);

scada::QualifiedName Convert(const OpcUa_QualifiedName& source);
void Convert(const scada::QualifiedName& source, OpcUa_QualifiedName& target);

scada::LocalizedText Convert(const OpcUa_LocalizedText& source);
void Convert(const scada::LocalizedText& source, OpcUa_LocalizedText& target);

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
  for (size_t i = 0; i < values.size(); ++i)
    Convert(std::move(values[i]), result[i]);
  return result;
}