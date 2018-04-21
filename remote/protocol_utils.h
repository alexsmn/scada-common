#pragma once

#include <set>
#include <vector>

#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/event.h"
#include "core/node_attributes.h"
#include "core/status.h"
#include "core/view_service.h"
#include "remote/protocol.h"

scada::NodeId FromProto(const protocol::NodeId& source);
void ToProto(const scada::NodeId& source, protocol::NodeId& target);

scada::Variant FromProto(const protocol::Variant& source);
void ToProto(const scada::Variant& source, protocol::Variant& target);

scada::DataValue FromProto(const protocol::DataValue& source);
void ToProto(const scada::DataValue& source, protocol::DataValue& target);

scada::Status FromProto(const protocol::Status& source);
void ToProto(const scada::Status& source, protocol::Status& target);

scada::Event FromProto(const protocol::Event& source);
void ToProto(const scada::Event& source, protocol::Event& target);

scada::NodeClass FromProto(const protocol::NodeClass source);
protocol::NodeClass ToProto(const scada::NodeClass source);

scada::NodeAttributes FromProto(const protocol::Attributes& source);
void ToProto(const scada::NodeAttributes& source, protocol::Attributes& target);

scada::AttributeId FromProto(protocol::AttributeId source);
protocol::AttributeId ToProto(scada::AttributeId source);

scada::ReadValueId FromProto(const protocol::ReadValueId& source);
void ToProto(const scada::ReadValueId& source, protocol::ReadValueId& target);

scada::BrowseDirection FromProto(protocol::BrowseDirection source);
protocol::BrowseDirection ToProto(scada::BrowseDirection source);

scada::ReferenceDescription FromProto(
    const protocol::ReferenceDescription& source);
void ToProto(const scada::ReferenceDescription& source,
             protocol::ReferenceDescription& target);

scada::BrowseResult FromProto(const protocol::BrowseResult& source);
void ToProto(const scada::BrowseResult& source, protocol::BrowseResult& target);

scada::ModelChangeEvent FromProto(const protocol::ModelChangeEvent& source);
void ToProto(const scada::ModelChangeEvent& source, protocol::ModelChangeEvent& target);

template <typename Target, typename Source>
inline std::vector<Target> VectorFromProto(
    const ::google::protobuf::RepeatedPtrField<Source>& source) {
  std::vector<Target> result;
  result.reserve(source.size());
  for (auto& p : source)
    result.emplace_back(FromProto(p));
  return result;
}

template <typename Target, typename Source>
inline std::vector<Target> BrowseVectorFromProto(
    const ::google::protobuf::RepeatedPtrField<Source>& source) {
  std::vector<Target> result;
  result.reserve(source.size());
  for (auto& p : source)
    result.emplace_back(BrowseFromProto(p));
  return result;
}

template <typename Target, typename Container>
inline void ContainerToProto(
    const Container& source,
    ::google::protobuf::RepeatedPtrField<Target>& target) {
  target.Reserve(target.size() + source.size());
  for (auto& s : source)
    ToProto(s, *target.Add());
}

template <typename Source, typename Target>
inline void ToProto(const std::vector<Source>& sources,
                    ::google::protobuf::RepeatedPtrField<Target>& targets) {
  ContainerToProto(sources, targets);
}

template <typename Source, typename Target>
inline void ToProto(const std::set<Source>& sources,
                    ::google::protobuf::RepeatedPtrField<Target>& targets) {
  ContainerToProto(sources, targets);
}

template <typename Target, typename Source>
inline std::set<Target> SetFromProto(
    const ::google::protobuf::RepeatedPtrField<Source>& source) {
  std::set<Target> result;
  for (auto& p : source)
    result.emplace(FromProto(p));
  return result;
}
