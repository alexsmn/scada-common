#include "opcua_server.h"

#include "core/attribute_service.h"
#include "core/expanded_node_id.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/view_service.h"
#include "opcua/opcua_conversion.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <opcuapp/data_value.h>
#include <opcuapp/requests.h>
#include <opcuapp/status_code.h>

namespace {

void SetNodeClass(const scada::DataValue& data_value, OpcUa_NodeClass& result) {
  if (scada::IsGood(data_value.status_code))
    result = static_cast<OpcUa_NodeClass>(data_value.value.get<int32_t>());
}

template<class T, class R>
void SetValue(const scada::DataValue& data_value, R& result) {
  if (scada::IsGood(data_value.status_code))
    Convert(data_value.value.get<T>(), result);
}

void FillBrowseResultsTypeDefinitions(scada::ViewService& view_service,
    opcua::Span<const OpcUa_BrowseDescription> descriptions,
    opcua::Span<OpcUa_BrowseResult> results, const std::function<void()>& callback) {
  assert(descriptions.size() == results.size());

  std::vector<scada::BrowseDescription> nodes;
  nodes.reserve(descriptions.size());
  
  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];
    if (!(description.ResultMask & OpcUa_BrowseResultMask_TypeDefinition))
      continue;

    auto& result = results[i];
    if (!opcua::StatusCode{result.StatusCode})
      continue;

    for (auto& reference : opcua::MakeSpan(result.References, static_cast<size_t>(result.NoOfReferences))) {
      auto node_id = Convert(reference.NodeId).node_id();
      assert(!node_id.is_null());
      nodes.push_back(scada::BrowseDescription{
          std::move(node_id),
          scada::BrowseDirection::Forward,
          OpcUaId_HasTypeDefinition,
          false
      });
    }
  }

  if (nodes.empty())
    return callback();

  view_service.Browse(nodes,
      [descriptions, results, callback]
      (const scada::Status& status, std::vector<scada::BrowseResult> browse_results) {
        if (!status)
          return callback();

        size_t browse_index = 0;
        for (size_t i = 0; i < descriptions.size(); ++i) {
          auto& description = descriptions[i];
          if (!(description.ResultMask & OpcUa_BrowseResultMask_TypeDefinition))
            continue;

          auto& result = results[i];
          if (!opcua::StatusCode{result.StatusCode})
            continue;

          for (auto& reference : opcua::MakeSpan(result.References, static_cast<size_t>(result.NoOfReferences))) {
            auto& browse_result = browse_results[browse_index++];
            if (!scada::IsGood(browse_result.status_code))
              continue;
            if (browse_result.references.size() != 1)
              continue;

            assert(browse_result.references[0].forward);
            assert(browse_result.references[0].reference_type_id == OpcUaId_HasTypeDefinition);
            auto& type_definition_id = browse_result.references[0].node_id;
            assert(!type_definition_id.is_null());
            Convert(scada::ExpandedNodeId{browse_result.references[0].node_id, {}, 0}, reference.TypeDefinition);
          }
        }

        callback();
      });
}

void FillBrowseResultsAttributes(scada::AttributeService& attribute_service,
    opcua::Span<const OpcUa_BrowseDescription> descriptions,
    opcua::Span<OpcUa_BrowseResult> results, const std::function<void()>& callback) {
  assert(descriptions.size() == results.size());

  std::vector<scada::ReadValueId> read_ids;
  read_ids.reserve(16);

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];
    auto& result = results[i];
    if (!opcua::StatusCode{result.StatusCode})
      continue;

    for (auto& reference : opcua::MakeSpan(result.References, static_cast<size_t>(result.NoOfReferences))) {
      auto node_id = Convert(reference.NodeId).node_id();
      assert(!node_id.is_null());
      if (description.ResultMask & OpcUa_BrowseResultMask_NodeClass)
        read_ids.emplace_back(node_id, scada::AttributeId::NodeClass);
      if (description.ResultMask & OpcUa_BrowseResultMask_BrowseName)
        read_ids.emplace_back(node_id, scada::AttributeId::BrowseName);
      if (description.ResultMask & OpcUa_BrowseResultMask_DisplayName)
        read_ids.emplace_back(node_id, scada::AttributeId::DisplayName);
    }
  }

  if (read_ids.empty())
    return callback();

  attribute_service.Read(read_ids,
      [descriptions, results, callback]
      (const scada::Status& status, std::vector<scada::DataValue> values) {
        if (!status)
          return callback();

        size_t index = 0;
        for (size_t i = 0; i < descriptions.size(); ++i) {
          auto& description = descriptions[i];
          auto& result = results[i];
          if (!opcua::StatusCode{result.StatusCode})
            continue;

          for (auto& reference : opcua::MakeSpan(result.References, static_cast<size_t>(result.NoOfReferences))) {
            if (description.ResultMask & OpcUa_BrowseResultMask_NodeClass)
              SetNodeClass(values[index++], reference.NodeClass);
            if (description.ResultMask & OpcUa_BrowseResultMask_BrowseName)
              SetValue<scada::QualifiedName>(values[index++], reference.BrowseName);
            if (description.ResultMask & OpcUa_BrowseResultMask_DisplayName)
              SetValue<scada::LocalizedText>(values[index++], reference.DisplayName);
          }
        }

        callback();
      });
}

class MonitoredItemAdapter : public opcua::server::MonitoredItem {
 public:
  explicit MonitoredItemAdapter(std::unique_ptr<scada::MonitoredItem> monitored_item)
      : monitored_item_{std::move(monitored_item)} {
  }

  ~MonitoredItemAdapter() {
  }

  // opcua::server::MonitoredItem

  virtual void SubscribeDataChange(const opcua::server::DataChangeHandler& data_change_handler) override {
    monitored_item_->set_data_change_handler([this, data_change_handler](const scada::DataValue& data_value) {
      opcua::DataValue ua_data_value;
      Convert(scada::DataValue{data_value}, ua_data_value.get());
      data_change_handler(std::move(ua_data_value));
    });
    monitored_item_->Subscribe();
  }

  virtual void SubscribeEvents(const opcua::server::EventHandler& event_handler) override {
    monitored_item_->set_event_handler([this, event_handler](const scada::Status& status, const scada::Event& event) {
      event_handler();
    });
    monitored_item_->Subscribe();
  }

 private:
  std::unique_ptr<scada::MonitoredItem> monitored_item_;
};

} // namespace

OpcUaServer::OpcUaServer(OpcUaServerContext&& context)
    : OpcUaServerContext{std::move(context)} {
  endpoint_.set_read_handler([this](OpcUa_ReadRequest& request, const opcua::server::ReadCallback& callback) {
    Read(request, callback);
  });

  endpoint_.set_browse_handler([this](OpcUa_BrowseRequest& request, const opcua::server::BrowseCallback& callback) {
    Browse(request, callback);
  });

  endpoint_.set_create_monitored_item_handler([this](opcua::ReadValueId& read_value_id) {
    return CreateMonitoredItem(read_value_id);
  });

  opcua::String url = "opc.tcp://localhost:4840";
  endpoint_.Open(std::move(url), true, server_certificate_.get(), server_private_key_, &pki_config_,
      {&security_policy_, 1});
}

OpcUaServer::~OpcUaServer() {
}

void OpcUaServer::Read(OpcUa_ReadRequest& request, const opcua::server::ReadCallback& callback) {
  auto timestamps_to_return = request.TimestampsToReturn;
  attribute_service_.Read(
      ConvertVector<scada::ReadValueId>(opcua::MakeSpan(request.NodesToRead, request.NoOfNodesToRead)),
      [timestamps_to_return, callback](const scada::Status& status, std::vector<scada::DataValue> values) {
        opcua::ReadResponse response;
        response.ResponseHeader.ServiceResult = MakeStatusCode(status.code()).code();
        opcua::Vector<OpcUa_DataValue> ua_values(values.size());
        for (size_t i = 0; i < ua_values.size(); ++i) {
          auto& ua_value = ua_values[i];
          Convert(std::move(values[i]), ua_value);
          if (timestamps_to_return != OpcUa_TimestampsToReturn_Source &&
              timestamps_to_return != OpcUa_TimestampsToReturn_Both) {
            OpcUa_DateTime_Clear(&ua_value.SourceTimestamp);
            ua_value.SourcePicoseconds = 0;
          }
          if (timestamps_to_return != OpcUa_TimestampsToReturn_Server &&
              timestamps_to_return != OpcUa_TimestampsToReturn_Both) {
            OpcUa_DateTime_Clear(&ua_value.ServerTimestamp);
            ua_value.ServerPicoseconds = 0;
          }
        }
        response.NoOfResults = ua_values.size();
        response.Results = ua_values.release();
        callback(response);
      });
}

void OpcUaServer::Browse(OpcUa_BrowseRequest& request, const opcua::server::BrowseCallback& callback) {
  auto descriptions = std::make_shared<opcua::Vector<OpcUa_BrowseDescription>>(
      opcua::Attach{}, request.NodesToBrowse, request.NoOfNodesToBrowse);
  request.NodesToBrowse = OpcUa_Null;
  request.NoOfNodesToBrowse = 0;

  view_service_.Browse(
      ConvertVector<scada::BrowseDescription>(*descriptions),
      [this, descriptions, callback](const scada::Status& status, std::vector<scada::BrowseResult> results) {
        if (!status) {
          opcua::BrowseResponse response;
          response.ResponseHeader.ServiceResult = MakeStatusCode(status.code()).code();
          callback(response);
          return;
        }

        auto ua_results = std::make_shared<opcua::Vector<OpcUa_BrowseResult>>(
            MakeVector<OpcUa_BrowseResult>(opcua::MakeSpan(results.data(), results.size())));
        FillBrowseResultsTypeDefinitions(view_service_,
            opcua::Span<const OpcUa_BrowseDescription>(descriptions->data(), descriptions->size()),
            opcua::Span<OpcUa_BrowseResult>(ua_results->data(), ua_results->size()),
            [this, descriptions, ua_results, callback] {
              FillBrowseResultsAttributes(attribute_service_,
                  opcua::Span<const OpcUa_BrowseDescription>(descriptions->data(), descriptions->size()),
                  opcua::Span<OpcUa_BrowseResult>(ua_results->data(), ua_results->size()),
                  [descriptions, ua_results, callback] {
                    opcua::BrowseResponse response;
                    response.ResponseHeader.ServiceResult = OpcUa_Good;
                    response.NoOfResults = ua_results->size();
                    response.Results = ua_results->release();
                    callback(response);
                  });
              });
            });
}

opcua::server::CreateMonitoredItemResult OpcUaServer::CreateMonitoredItem(opcua::ReadValueId& read_value_id) {
  auto id = Convert(read_value_id);
  auto monitored_item = monitored_item_service_.CreateMonitoredItem(id.first, id.second);
  if (!monitored_item)
    return {OpcUa_Bad};
  return {OpcUa_Good, std::make_unique<MonitoredItemAdapter>(std::move(monitored_item))};
}

std::chrono::milliseconds GetDateTimeDiff(OpcUa_DateTime a_Value1, OpcUa_DateTime a_Value2) {
  auto ullValue1 = (static_cast<int64_t>(a_Value1.dwHighDateTime) << 32) + a_Value1.dwLowDateTime;
  auto ullValue2 = (static_cast<int64_t>(a_Value2.dwHighDateTime) << 32) + a_Value2.dwLowDateTime;
  auto ullResult = ullValue2 - ullValue1;
  return std::chrono::milliseconds{ ullResult * 10 };
}
