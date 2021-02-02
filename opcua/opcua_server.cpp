#include "opcua_server.h"

#include "core/attribute_service.h"
#include "core/expanded_node_id.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/node_management_service.h"
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

template <class T, class R>
void SetValue(const scada::DataValue& data_value, R& result) {
  if (scada::IsGood(data_value.status_code))
    Convert(data_value.value.get<T>(), result);
}

void FillBrowseResultsTypeDefinitions(
    scada::ViewService& view_service,
    opcua::Span<const OpcUa_BrowseDescription> descriptions,
    opcua::Span<OpcUa_BrowseResult> results,
    const std::function<void()>& callback) {
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

    auto references = opcua::MakeSpan(
        result.References, static_cast<size_t>(result.NoOfReferences));
    for (auto& reference : references) {
      auto node_id = Convert(reference.NodeId).node_id();
      assert(!node_id.is_null());
      nodes.push_back(scada::BrowseDescription{
          std::move(node_id), scada::BrowseDirection::Forward,
          OpcUaId_HasTypeDefinition, false});
    }
  }

  if (nodes.empty())
    return callback();

  view_service.Browse(
      nodes, [descriptions, results, callback](
                 const scada::Status& status,
                 std::vector<scada::BrowseResult> browse_results) {
        if (!status)
          return callback();

        size_t browse_index = 0;
        for (size_t i = 0; i < descriptions.size(); ++i) {
          auto& description = descriptions[i];
          auto& result = results[i];

          if (!(description.ResultMask & OpcUa_BrowseResultMask_TypeDefinition))
            continue;

          if (!opcua::StatusCode{result.StatusCode})
            continue;

          auto references = opcua::MakeSpan(
              result.References, static_cast<size_t>(result.NoOfReferences));
          for (auto& reference : references) {
            auto& browse_result = browse_results[browse_index++];
            if (!scada::IsGood(browse_result.status_code))
              continue;
            if (browse_result.references.size() != 1)
              continue;

            assert(browse_result.references[0].forward);
            assert(browse_result.references[0].reference_type_id ==
                   OpcUaId_HasTypeDefinition);
            auto& type_definition_id = browse_result.references[0].node_id;
            assert(!type_definition_id.is_null());
            Convert(scada::ExpandedNodeId{type_definition_id, {}, 0},
                    reference.TypeDefinition);
          }
        }

        callback();
      });
}

void FillBrowseResultsAttributes(
    scada::AttributeService& attribute_service,
    opcua::Span<const OpcUa_BrowseDescription> descriptions,
    opcua::Span<OpcUa_BrowseResult> results,
    const std::function<void()>& callback) {
  assert(descriptions.size() == results.size());

  std::vector<scada::ReadValueId> read_ids;
  read_ids.reserve(16);

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];
    auto& result = results[i];
    if (!opcua::StatusCode{result.StatusCode})
      continue;

    auto references = opcua::MakeSpan(
        result.References, static_cast<size_t>(result.NoOfReferences));
    for (auto& reference : references) {
      auto node_id = Convert(reference.NodeId).node_id();
      assert(!node_id.is_null());
      if (description.ResultMask & OpcUa_BrowseResultMask_NodeClass)
        read_ids.push_back({node_id, scada::AttributeId::NodeClass});
      if (description.ResultMask & OpcUa_BrowseResultMask_BrowseName)
        read_ids.push_back({node_id, scada::AttributeId::BrowseName});
      if (description.ResultMask & OpcUa_BrowseResultMask_DisplayName)
        read_ids.push_back({node_id, scada::AttributeId::DisplayName});
    }
  }

  if (read_ids.empty())
    return callback();

  attribute_service.Read(read_ids, [descriptions, results, callback](
                                       const scada::Status& status,
                                       std::vector<scada::DataValue> values) {
    if (!status)
      return callback();

    size_t index = 0;
    for (size_t i = 0; i < descriptions.size(); ++i) {
      auto& description = descriptions[i];
      auto& result = results[i];
      if (!opcua::StatusCode{result.StatusCode})
        continue;

      auto references = opcua::MakeSpan(
          result.References, static_cast<size_t>(result.NoOfReferences));
      for (auto& reference : references) {
        if (description.ResultMask & OpcUa_BrowseResultMask_NodeClass)
          SetNodeClass(values[index++], reference.NodeClass);
        if (description.ResultMask & OpcUa_BrowseResultMask_BrowseName)
          SetValue<scada::QualifiedName>(values[index++], reference.BrowseName);
        if (description.ResultMask & OpcUa_BrowseResultMask_DisplayName)
          SetValue<scada::LocalizedText>(values[index++],
                                         reference.DisplayName);
      }
    }

    callback();
  });
}

class MonitoredItemAdapter : public opcua::server::MonitoredItem {
 public:
  explicit MonitoredItemAdapter(
      std::shared_ptr<scada::MonitoredItem> monitored_item)
      : monitored_item_{std::move(monitored_item)} {}

  ~MonitoredItemAdapter() {}

  // opcua::server::MonitoredItem

  virtual void SubscribeDataChange(
      const opcua::server::DataChangeHandler& data_change_handler) override {
    monitored_item_->set_data_change_handler(
        [this, data_change_handler](const scada::DataValue& data_value) {
          opcua::DataValue ua_data_value;
          Convert(scada::DataValue{data_value}, ua_data_value.get());
          data_change_handler(std::move(ua_data_value));
        });
    monitored_item_->Subscribe();
  }

  virtual void SubscribeEvents(
      const opcua::server::EventHandler& event_handler) override {
    monitored_item_->set_event_handler(
        [this, event_handler](const scada::Status& status,
                              const std::any& event) { event_handler({}); });
    monitored_item_->Subscribe();
  }

 private:
  const std::shared_ptr<scada::MonitoredItem> monitored_item_;
};

}  // namespace

opcua::UInt32 ParseTraceLevel(std::string_view str) {
  const std::pair<std::string_view, opcua::UInt32> kTraceLevels[] = {
      {"error", OPCUA_TRACE_OUTPUT_LEVEL_ERROR},
      {"warning", OPCUA_TRACE_OUTPUT_LEVEL_WARNING},
      {"system", OPCUA_TRACE_OUTPUT_LEVEL_SYSTEM},
      {"info", OPCUA_TRACE_OUTPUT_LEVEL_INFO},
      {"debug", OPCUA_TRACE_OUTPUT_LEVEL_DEBUG},
      {"content", OPCUA_TRACE_OUTPUT_LEVEL_CONTENT},
      {"all", OPCUA_TRACE_OUTPUT_LEVEL_ALL},
      {"none", OPCUA_TRACE_OUTPUT_LEVEL_NONE},
  };

  for (auto& p : kTraceLevels) {
    if (p.first == str)
      return p.second;
  }

  return OPCUA_TRACE_OUTPUT_LEVEL_NONE;
}

// OpcUaServer

OpcUaServer::OpcUaServer(OpcUaServerContext&& context)
    : OpcUaServerContext{std::move(context)},
      proxy_stub_{platform_, MakeProxyStubConfiguration()} {
  endpoint_.set_session_handlers({
      [this](OpcUa_ReadRequest& request,
             const opcua::server::ReadCallback& callback) {
        Read(request, callback);
      },
      [this](
          OpcUa_WriteRequest& request,
          const opcua::server::SimpleCallback<OpcUa_WriteResponse>& callback) {
        Write(request, callback);
      },
      [this](OpcUa_BrowseRequest& request,
             const opcua::server::BrowseCallback& callback) {
        Browse(request, callback);
      },
      [this](OpcUa_TranslateBrowsePathsToNodeIdsRequest& request,
             const opcua::server::TranslateBrowsePathsToNodeIdsCallback&
                 callback) { TranslateBrowsePaths(request, callback); },
      [this](opcua::ReadValueId&& read_value_id,
             opcua::MonitoringParameters&& params) {
        return CreateMonitoredItem(std::move(read_value_id), std::move(params));
      },
      [this](OpcUa_AddNodesRequest& request,
             const opcua::server::SimpleCallback<OpcUa_AddNodesResponse>&
                 callback) { AddNodes(request, callback); },
      [this](OpcUa_DeleteNodesRequest& request,
             const opcua::server::SimpleCallback<OpcUa_DeleteNodesResponse>&
                 callback) { DeleteNodes(request, callback); },
  });

  endpoint_.set_product_uri("TelecontrolScada");
  endpoint_.set_application_uri("TelecontrolScadaServer");
  endpoint_.set_application_name("Telecontrol SCADA Server");

  LOG_INFO(logger_) << "Opening endpoint..." << LOG_TAG("Url", url_);
  try {
    endpoint_.Open(url_.c_str(), true, server_certificate_.get(),
                   server_private_key_.get(), &pki_config_,
                   {&security_policy_, 1});
  } catch (const opcua::StatusCodeException& e) {
    LOG_ERROR(logger_) << "Open endpoint error"
                       << LOG_TAG("StatusCode", e.status_code().code());
    throw e;
  }
}

OpcUaServer::~OpcUaServer() {
  LOG_INFO(logger_) << "Destroying";
}

void OpcUaServer::Read(OpcUa_ReadRequest& request,
                       const opcua::server::ReadCallback& callback) {
  auto timestamps_to_return = request.TimestampsToReturn;
  attribute_service_.Read(
      ConvertVector<scada::ReadValueId>(
          opcua::MakeSpan(request.NodesToRead, request.NoOfNodesToRead)),
      [timestamps_to_return, callback](scada::Status&& status,
                                       std::vector<scada::DataValue> values) {
        opcua::ReadResponse response;
        response.ResponseHeader.ServiceResult =
            MakeStatusCode(status.code()).code();
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
        callback(std::move(response));
      });
}

void OpcUaServer::Write(
    OpcUa_WriteRequest& request,
    const opcua::server::SimpleCallback<OpcUa_WriteResponse>& callback) {
  // TODO: UserID.
  attribute_service_.Write(ConvertVector<scada::WriteValueId>(opcua::MakeSpan(
                               request.NodesToWrite, request.NoOfNodesToWrite)),
                           {},
                           [callback](scada::Status&& status,
                                      std::vector<scada::StatusCode> results) {
                             opcua::WriteResponse response;
                             response.ResponseHeader.ServiceResult =
                                 MakeStatusCode(status.code()).code();
                             auto opcua_results =
                                 ConvertStatusCodesFromVector(results);
                             response.NoOfResults = opcua_results.size();
                             response.Results = opcua_results.release();
                             callback(std::move(response));
                           });
}

void OpcUaServer::Browse(OpcUa_BrowseRequest& request,
                         const opcua::server::BrowseCallback& callback) {
  auto descriptions = std::make_shared<opcua::Vector<OpcUa_BrowseDescription>>(
      opcua::Attach{}, request.NodesToBrowse, request.NoOfNodesToBrowse);
  request.NodesToBrowse = OpcUa_Null;
  request.NoOfNodesToBrowse = 0;

  view_service_.Browse(
      ConvertVector<scada::BrowseDescription>(*descriptions),
      [this, descriptions, callback](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        if (!status) {
          opcua::BrowseResponse response;
          response.ResponseHeader.ServiceResult =
              MakeStatusCode(status.code()).code();
          callback(std::move(response));
          return;
        }

        auto opcua_results =
            std::make_shared<opcua::Vector<OpcUa_BrowseResult>>(
                MakeVector<OpcUa_BrowseResult>(
                    opcua::MakeSpan(results.data(), results.size())));

        FillBrowseResultsTypeDefinitions(
            view_service_, *descriptions, *opcua_results,
            [this, descriptions, opcua_results, callback] {
              FillBrowseResultsAttributes(
                  attribute_service_, *descriptions, *opcua_results,
                  [descriptions, opcua_results, callback] {
                    opcua::BrowseResponse response;
                    response.ResponseHeader.ServiceResult = OpcUa_Good;
                    response.NoOfResults = opcua_results->size();
                    response.Results = opcua_results->release();
                    callback(std::move(response));
                  });
            });
      });
}

void OpcUaServer::TranslateBrowsePaths(
    OpcUa_TranslateBrowsePathsToNodeIdsRequest& request,
    const opcua::server::TranslateBrowsePathsToNodeIdsCallback& callback) {
  view_service_.TranslateBrowsePaths(
      ConvertVector<scada::BrowsePath>(
          opcua::MakeSpan(request.BrowsePaths, request.NoOfBrowsePaths)),
      [callback](scada::Status&& status,
                 std::vector<scada::BrowsePathResult>&& results) {
        opcua::TranslateBrowsePathsToNodeIdsResponse response;
        response.ResponseHeader.ServiceResult =
            MakeStatusCode(status.code()).code();
        auto opcua_results = ConvertFromVector<OpcUa_BrowsePathResult>(results);
        response.NoOfResults = opcua_results.size();
        response.Results = opcua_results.release();
        callback(std::move(response));
      });
}

void OpcUaServer::AddNodes(
    OpcUa_AddNodesRequest& request,
    const opcua::server::SimpleCallback<OpcUa_AddNodesResponse>& callback) {
  node_management_service_.AddNodes(
      ConvertVector<scada::AddNodesItem>(
          opcua::MakeSpan(request.NodesToAdd, request.NoOfNodesToAdd)),
      [callback](scada::Status&& status,
                 std::vector<scada::AddNodesResult>&& results) {
        opcua::AddNodesResponse response;
        response.ResponseHeader.ServiceResult =
            MakeStatusCode(status.code()).code();
        response.NoOfResults = results.size();
        response.Results =
            ConvertFromVector<OpcUa_AddNodesResult>(results).release();
        callback(std::move(response));
      });
}

void OpcUaServer::DeleteNodes(
    OpcUa_DeleteNodesRequest& request,
    const opcua::server::SimpleCallback<OpcUa_DeleteNodesResponse>& callback) {
  node_management_service_.DeleteNodes(
      ConvertVector<scada::DeleteNodesItem>(
          opcua::MakeSpan(request.NodesToDelete, request.NoOfNodesToDelete)),
      [callback](scada::Status&& status,
                 std::vector<scada::StatusCode>&& results) {
        opcua::DeleteNodesResponse response;
        response.ResponseHeader.ServiceResult =
            MakeStatusCode(status.code()).code();
        auto opcua_results = ConvertStatusCodesFromVector(results);
        response.NoOfResults = opcua_results.size();
        response.Results = opcua_results.release();
        callback(std::move(response));
      });
}

opcua::server::CreateMonitoredItemResult OpcUaServer::CreateMonitoredItem(
    opcua::ReadValueId&& read_value_id,
    opcua::MonitoringParameters&& params) {
  auto scada_id = Convert(std::move(read_value_id));
  auto scada_params = Convert(std::move(params));
  auto monitored_item = monitored_item_service_.CreateMonitoredItem(
      std::move(scada_id), std::move(scada_params));
  if (!monitored_item)
    return {OpcUa_Bad};
  return {OpcUa_Good,
          std::make_shared<MonitoredItemAdapter>(std::move(monitored_item))};
}

opcua::ProxyStubConfiguration OpcUaServer::MakeProxyStubConfiguration() {
  opcua::ProxyStubConfiguration result;
  result.bProxyStub_Trace_Enabled =
      trace_level_ != OPCUA_TRACE_OUTPUT_LEVEL_NONE;
  result.uProxyStub_Trace_Level = trace_level_;
  result.iSerializer_MaxAlloc = -1;
  result.iSerializer_MaxStringLength = -1;
  result.iSerializer_MaxByteStringLength = -1;
  result.iSerializer_MaxArrayLength = -1;
  result.iSerializer_MaxMessageSize = -1;
  result.iSerializer_MaxRecursionDepth = -1;
  result.bSecureListener_ThreadPool_Enabled = OpcUa_False;
  result.iSecureListener_ThreadPool_MinThreads = -1;
  result.iSecureListener_ThreadPool_MaxThreads = -1;
  result.iSecureListener_ThreadPool_MaxJobs = -1;
  result.bSecureListener_ThreadPool_BlockOnAdd = OpcUa_True;
  result.uSecureListener_ThreadPool_Timeout = OPCUA_INFINITE;
  result.bTcpListener_ClientThreadsEnabled = OpcUa_False;
  result.iTcpListener_DefaultChunkSize = -1;
  result.iTcpConnection_DefaultChunkSize = -1;
  result.iTcpTransport_MaxMessageLength = -1;
  result.iTcpTransport_MaxChunkCount = -1;
  result.bTcpStream_ExpectWriteToBlock = OpcUa_True;
  return result;
}
