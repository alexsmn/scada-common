#pragma once

#include "base/time/time.h"
#include "common/node_state.h"
#include "scada/aggregate_filter.h"
#include "scada/attribute_ids.h"
#include "scada/attribute_service.h"
#include "scada/data_value.h"
#include "scada/event_filter.h"
#include "scada/expanded_node_id.h"
#include "scada/extension_object.h"
#include "scada/monitored_item_service.h"
#include "scada/monitoring_parameters.h"
#include "scada/node_class.h"
#include "scada/node_management_service.h"
#include "scada/status.h"
#include "scada/variant.h"
#include "scada/view_service.h"

#include <opcuapp/basic_types.h>
#include <opcuapp/structs.h>
#include <opcuapp/vector.h>

template <class Target, class Source>
Target Convert(Source&& source);

scada::NodeClass ConvertNodeClass(opcua::NodeClass node_class);
opcua::NodeClass Convert(scada::NodeClass node_class);

scada::StatusCode ConvertStatusCode(OpcUa_StatusCode status_code);
opcua::StatusCode MakeStatusCode(scada::StatusCode status_code);

scada::Variant Convert(OpcUa_Variant&& source);
void Convert(scada::Variant&& source, OpcUa_Variant& target);

scada::Qualifier MakeQualifier(opcua::StatusCode source);

scada::String Convert(const OpcUa_String& source);
scada::String Convert(OpcUa_String&& source);
void Convert(const scada::String& source, OpcUa_String& target);

scada::DateTime Convert(OpcUa_DateTime source);
OpcUa_DateTime Convert(scada::DateTime source);

scada::DataValue Convert(OpcUa_DataValue&& source);
void Convert(scada::DataValue&& source, OpcUa_DataValue& target);

scada::NodeId Convert(const OpcUa_NodeId& node_id);
void Convert(const scada::NodeId& source, OpcUa_NodeId& target);

scada::ExpandedNodeId Convert(const OpcUa_ExpandedNodeId& node_id);
void Convert(const scada::ExpandedNodeId& source, OpcUa_ExpandedNodeId& target);

scada::AttributeId ConvertAttributeId(OpcUa_Int32 attribute_id);
OpcUa_Int32 Convert(scada::AttributeId attribute_id);

scada::ReadValueId Convert(const OpcUa_ReadValueId& source);
scada::ReadValueId Convert(OpcUa_ReadValueId&& source);
void Convert(const scada::ReadValueId& source, OpcUa_ReadValueId& target);

scada::AggregateFilter Convert(OpcUa_AggregateFilter&& source);
void Convert(const scada::AggregateFilter& source,
             OpcUa_AggregateFilter& target);

scada::MonitoringParameters Convert(OpcUa_MonitoringParameters&& source);
void Convert(const scada::MonitoringParameters& source,
             OpcUa_MonitoringParameters& target);

OpcUa_BrowseDirection Convert(scada::BrowseDirection source);
scada::BrowseDirection Convert(OpcUa_BrowseDirection source);

scada::BrowseDescription Convert(const OpcUa_BrowseDescription& source);
OpcUa_BrowseDescription Convert(const scada::BrowseDescription& source);

scada::ReferenceDescription Convert(const OpcUa_ReferenceDescription& source);
void Convert(const scada::ReferenceDescription& source,
             OpcUa_ReferenceDescription& target);

scada::BrowseResult Convert(const OpcUa_BrowseResult& source);
void Convert(const scada::BrowseResult& source, OpcUa_BrowseResult& target);

scada::ExtensionObject Convert(OpcUa_ExtensionObject&& object);
void Convert(scada::ExtensionObject&& source, OpcUa_ExtensionObject& target);

scada::ByteString Convert(const OpcUa_ByteString& source);
void Convert(const scada::ByteString& source, OpcUa_ByteString& target);

scada::QualifiedName Convert(const OpcUa_QualifiedName& source);
void Convert(const scada::QualifiedName& source, OpcUa_QualifiedName& target);

scada::LocalizedText Convert(const OpcUa_LocalizedText& source);
void Convert(const scada::LocalizedText& source, OpcUa_LocalizedText& target);

scada::AddNodesItem Convert(const OpcUa_AddNodesItem& source);
void Convert(const scada::AddNodesResult& source, OpcUa_AddNodesResult& target);

scada::DeleteNodesItem Convert(const OpcUa_DeleteNodesItem& source);

scada::WriteValue Convert(OpcUa_WriteValue&& source);
void Convert(scada::WriteValue&& source, OpcUa_WriteValue& target);

scada::BrowsePath Convert(OpcUa_BrowsePath&& source);
void Convert(scada::BrowsePathResult&& source, OpcUa_BrowsePathResult& target);

scada::EventFilter Convert(const OpcUa_EventFilter& source);
void Convert(const scada::EventFilter& source, OpcUa_EventFilter& target);

template <typename T, class It>
inline std::vector<T> ConvertVector(It first, It last) {
  std::vector<T> result(std::distance(first, last));
  std::transform(first, last, result.begin(),
                 [](auto&& source) { return Convert(std::move(source)); });
  return result;
}

template <typename T, class It>
inline std::vector<T> ConvertVectorCopy(It first, It last) {
  std::vector<T> result(std::distance(first, last));
  std::transform(first, last, result.begin(),
                 [](const auto& source) { return Convert(source); });
  return result;
}

template <typename T, class Range>
inline std::vector<T> ConvertVector(Range&& range) {
  return ConvertVector<T>(std::begin(range), std::end(range));
}

template <typename T, class Range>
inline opcua::Vector<T> ConvertFromVector(Range&& range) {
  opcua::Vector<T> result(std::size(range));
  for (size_t i = 0; i < std::size(range); ++i)
    Convert(std::move(range[i]), result[i]);
  return result;
}

inline std::vector<scada::StatusCode> ConvertStatusCodeVector(
    opcua::Span<const OpcUa_StatusCode> range) {
  std::vector<scada::StatusCode> result(std::size(range));
  for (size_t i = 0; i < std::size(range); ++i)
    result[i] = ConvertStatusCode(range[i]);
  return result;
}

inline opcua::Vector<OpcUa_StatusCode> ConvertStatusCodesFromVector(
    const std::vector<scada::StatusCode>& range) {
  opcua::Vector<OpcUa_StatusCode> result(std::size(range));
  for (size_t i = 0; i < std::size(range); ++i)
    result[i] = MakeStatusCode(range[i]).code();
  return result;
}

template <typename T, typename S>
opcua::Vector<T> MakeVector(opcua::Span<S> values) {
  opcua::Vector<T> result(values.size());
  for (size_t i = 0; i < values.size(); ++i)
    Convert(std::move(values[i]), result[i]);
  return result;
}
