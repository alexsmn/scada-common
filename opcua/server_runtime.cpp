#include "opcua/server_runtime.h"

#include "base/awaitable_promise.h"
#include "base/boost_log.h"
#include "base/executor_util.h"
#include "base/promise.h"
#include "common/coroutine_service_resolver.h"
#include "scada/coroutine_services.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace opcua {

namespace {

BoostLogger logger_{LOG_NAME("ServerRuntime")};

template <typename Response>
Response SessionMissingResponse() {
  return {.status = scada::StatusCode::Bad_SessionIsLoggedOff};
}

template <>
ResponseBody SessionMissingResponse<ResponseBody>() {
  return ServiceFault{scada::StatusCode::Bad_SessionIsLoggedOff};
}

DataServicesServerRuntimeContext MakeDataServicesServerRuntimeContext(
    ServerRuntimeContext&& context) {
  return DataServicesServerRuntimeContext{
      .executor = std::move(context.executor),
      .session_manager = context.session_manager,
      .data_services = DataServices::FromUnownedServices(scada::services{
          .attribute_service = &context.attribute_service,
          .monitored_item_service = &context.monitored_item_service,
          .method_service = &context.method_service,
          .history_service = &context.history_service,
          .view_service = &context.view_service,
          .node_management_service = &context.node_management_service}),
      .now = std::move(context.now),
      .post_delayed_task = std::move(context.post_delayed_task)};
}

}  // namespace

ServerRuntime::ServerRuntime(ServerRuntimeContext&& context)
    : ServerRuntime{MakeDataServicesServerRuntimeContext(std::move(context))} {}

ServerRuntime::ServerRuntime(CoroutineServerRuntimeContext&& context)
    : executor_{std::move(context.executor)},
      session_manager_{context.session_manager},
      monitored_item_service_{context.monitored_item_service},
      attribute_service_{context.attribute_service},
      view_service_{context.view_service},
      history_service_{context.history_service},
      method_service_{context.method_service},
      node_management_service_{context.node_management_service},
      now_{std::move(context.now)},
      post_delayed_task_{std::move(context.post_delayed_task)} {}

ServerRuntime::ServerRuntime(DataServicesServerRuntimeContext&& context)
    : data_services_{std::move(context.data_services)},
      executor_{std::move(context.executor)},
      session_manager_{context.session_manager},
      monitored_item_service_{
          scada::service_resolver::RequireSharedService(
              data_services_.monitored_item_service_)},
      attribute_service_{
          scada::service_resolver::RequireCoroutineService(
              executor_, data_services_.coroutine_attribute_service_,
              data_services_.attribute_service_, attribute_service_adapter_)},
      view_service_{
          scada::service_resolver::RequireCoroutineService(
              executor_, data_services_.coroutine_view_service_,
              data_services_.view_service_, view_service_adapter_)},
      history_service_{
          scada::service_resolver::RequireCoroutineService(
              executor_, data_services_.coroutine_history_service_,
              data_services_.history_service_, history_service_adapter_)},
      method_service_{
          scada::service_resolver::RequireCoroutineService(
              executor_, data_services_.coroutine_method_service_,
              data_services_.method_service_, method_service_adapter_)},
      node_management_service_{
          scada::service_resolver::RequireCoroutineService(
              executor_, data_services_.coroutine_node_management_service_,
              data_services_.node_management_service_,
              node_management_service_adapter_)},
      now_{std::move(context.now)},
      post_delayed_task_{std::move(context.post_delayed_task)} {}

ServerRuntime::~ServerRuntime() = default;

Awaitable<ServiceResponse> ServerRuntime::HandleServiceRequest(
    const ServerSession& session,
    ServiceRequest request) const {
  const auto user_id = session.GetServiceContext().user_id();
  co_return co_await ServiceHandler{
      {.attribute_service = attribute_service_,
       .view_service = view_service_,
       .history_service = history_service_,
       .method_service = method_service_,
       .node_management_service = node_management_service_,
       .user_id = user_id}}
      .Handle(std::move(request));
}

void ServerRuntime::Detach(ConnectionState& connection) {
  if (!connection.authentication_token.has_value())
    return;

  LOG_INFO(logger_) << "OPC UA runtime detaching connection session"
                    << LOG_TAG("AuthenticationToken",
                               connection.authentication_token->ToString());
  session_manager_.DetachSession(*connection.authentication_token);
  connection.authentication_token.reset();
}

ServerSession* ServerRuntime::FindSession(
    const scada::NodeId& authentication_token) const {
  const auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? it->second.get() : nullptr;
}

ServerSession* ServerRuntime::FindAttachedSession(
    const ConnectionState& connection) const {
  if (!connection.authentication_token.has_value())
    return nullptr;
  return FindSession(*connection.authentication_token);
}

void ServerRuntime::ForgetSession(const scada::NodeId& authentication_token) {
  LOG_INFO(logger_) << "OPC UA runtime forgetting session state"
                    << LOG_TAG("AuthenticationToken",
                               authentication_token.ToString());
  RemoveSessionSubscriptions(authentication_token);
  sessions_.erase(authentication_token);
}

void ServerRuntime::IndexSessionSubscriptions(
    const scada::NodeId& authentication_token,
    const ServerSession& session) {
  const auto subscription_ids = session.GetSubscriptionIds();
  for (const auto subscription_id : subscription_ids)
    subscription_owners_[subscription_id] = authentication_token;
  if (!subscription_ids.empty()) {
    next_subscription_id_ = std::max(
        next_subscription_id_,
        *std::max_element(subscription_ids.begin(), subscription_ids.end()) +
            1);
  }
}

void ServerRuntime::RemoveSessionSubscriptions(
    const scada::NodeId& authentication_token) {
  std::erase_if(subscription_owners_, [&](const auto& entry) {
    return entry.second == authentication_token;
  });
}

Awaitable<void> ServerRuntime::Delay(base::TimeDelta delay) const {
  if (delay <= base::TimeDelta{})
    co_return;

  promise<void> delayed;
  auto callback = [delayed]() mutable { delayed.resolve(); };
  if (post_delayed_task_) {
    post_delayed_task_(delay, std::move(callback));
  } else {
    PostDelayedTask(executor_,
                    std::chrono::milliseconds{delay.InMilliseconds()},
                    std::move(callback));
  }
  co_await AwaitPromise(executor_, std::move(delayed));
}

Awaitable<ResponseBody> ServerRuntime::Handle(ConnectionState& connection,
                                              RequestBody request) {
  auto body = co_await std::visit(
      [this, &connection](auto&& typed_request) -> Awaitable<ResponseBody> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, CreateSessionRequest>) {
          co_return ResponseBody{co_await session_manager_.CreateSession(
              std::move(typed_request))};
        } else if constexpr (std::is_same_v<T, ActivateSessionRequest>) {
          co_return co_await HandleActivateSession(connection,
                                                   std::move(typed_request));
        } else if constexpr (std::is_same_v<T, CloseSessionRequest>) {
          const auto response = session_manager_.CloseSession(typed_request);
          if (response.status)
            ForgetSession(typed_request.authentication_token);
          if (connection.authentication_token ==
              typed_request.authentication_token)
            connection.authentication_token.reset();
          co_return ResponseBody{response};
        } else if constexpr (std::is_same_v<T, CreateSubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<CreateSubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->CreateSubscriptionWithId(
              next_subscription_id_++, typed_request);
          subscription_owners_[response.subscription_id] =
              *connection.authentication_token;
          co_return ResponseBody{response};
        } else if constexpr (std::is_same_v<T, ModifySubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<ModifySubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->ModifySubscription(typed_request)};
        } else if constexpr (std::is_same_v<T, SetPublishingModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<SetPublishingModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->SetPublishingMode(typed_request)};
        } else if constexpr (std::is_same_v<T, DeleteSubscriptionsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<DeleteSubscriptionsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->DeleteSubscriptions(typed_request);
          for (size_t i = 0; i < typed_request.subscription_ids.size(); ++i) {
            if (response.results[i] == scada::StatusCode::Good)
              subscription_owners_.erase(typed_request.subscription_ids[i]);
          }
          co_return ResponseBody{response};
        } else if constexpr (std::is_same_v<T, PublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{SessionMissingResponse<PublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          auto ack_results = session->AcknowledgePublishRequest(typed_request);
          for (;;) {
            if (connection.closed) {
              co_return ResponseBody{SessionMissingResponse<PublishResponse>()};
            }

            session = FindAttachedSession(connection);
            if (!session) {
              co_return ResponseBody{SessionMissingResponse<PublishResponse>()};
            }

            // cppcheck-suppress nullPointerRedundantCheck
            auto poll = session->PollPublish();
            if (poll.response.has_value()) {
              poll.response->results = std::move(ack_results);
              co_return ResponseBody{std::move(*poll.response)};
            }
            if (!poll.wait_for.has_value()) {
              co_return ResponseBody{
                  PublishResponse{.status = scada::StatusCode::Good,
                                  .results = std::move(ack_results)}};
            }
            co_await Delay(*poll.wait_for);
          }
        } else if constexpr (std::is_same_v<T, RepublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{SessionMissingResponse<RepublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->Republish(typed_request)};
        } else if constexpr (std::is_same_v<T, TransferSubscriptionsRequest>) {
          auto* target_session = FindAttachedSession(connection);
          if (!target_session || !connection.authentication_token.has_value()) {
            co_return ResponseBody{
                SessionMissingResponse<TransferSubscriptionsResponse>()};
          }

          TransferSubscriptionsResponse response{.status =
                                                     scada::StatusCode::Good};
          response.results.assign(typed_request.subscription_ids.size(),
                                  scada::StatusCode::Bad_WrongSubscriptionId);
          std::unordered_map<scada::NodeId,
                             std::vector<std::pair<size_t, SubscriptionId>>>
              groups;

          for (size_t i = 0; i < typed_request.subscription_ids.size(); ++i) {
            const auto subscription_id = typed_request.subscription_ids[i];
            const auto owner_it = subscription_owners_.find(subscription_id);
            if (owner_it == subscription_owners_.end()) {
              response.results[i] = scada::StatusCode::Bad_WrongSubscriptionId;
              continue;
            }
            if (owner_it->second == *connection.authentication_token) {
              response.results[i] = scada::StatusCode::Good;
              continue;
            }
            groups[owner_it->second].push_back({i, subscription_id});
          }

          for (const auto& [source_token, group] : groups) {
            auto* source_session = FindSession(source_token);
            if (!source_session) {
              for (const auto& [index, subscription_id] : group)
                response.results[index] =
                    scada::StatusCode::Bad_SessionIsLoggedOff;
              continue;
            }

            TransferSubscriptionsRequest grouped_request;
            grouped_request.send_initial_values =
                typed_request.send_initial_values;
            for (const auto& [index, subscription_id] : group)
              grouped_request.subscription_ids.push_back(subscription_id);

            const auto grouped_response =
                target_session->TransferSubscriptionsFrom(*source_session,
                                                          grouped_request);
            for (size_t i = 0; i < group.size(); ++i) {
              response.results[group[i].first] = grouped_response.results[i];
              if (grouped_response.results[i] == scada::StatusCode::Good)
                subscription_owners_[group[i].second] =
                    *connection.authentication_token;
            }
          }

          co_return ResponseBody{response};
        } else if constexpr (std::is_same_v<T, CreateMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<CreateMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->CreateMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, ModifyMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<ModifyMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->ModifyMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, DeleteMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<DeleteMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->DeleteMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, SetMonitoringModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return ResponseBody{
                SessionMissingResponse<SetMonitoringModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->SetMonitoringMode(typed_request)};
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<ResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto& attached_session = *session;
          auto response = co_await HandleServiceRequest(
              attached_session, ServiceRequest{BrowseRequest{
                                    .inputs = std::move(typed_request.inputs),
                                }});
          if (!std::holds_alternative<BrowseResponse>(response))
            co_return SessionMissingResponse<ResponseBody>();
          auto browse = std::get<BrowseResponse>(std::move(response));
          auto paged_response = session->StoreBrowseResults(
              std::move(browse),
              typed_request.requested_max_references_per_node);
          co_return ResponseBody{std::move(paged_response)};
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<ResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          co_return ResponseBody{session->BrowseNext(typed_request)};
        } else {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<ResponseBody>();
          assert(session != nullptr);
          auto service_response = co_await HandleServiceRequest(
              *session, ServiceRequest{std::move(typed_request)});
          co_return std::visit(
              [](auto&& typed_response) -> ResponseBody {
                return ResponseBody{std::move(typed_response)};
              },
              std::move(service_response));
        }
      },
      std::move(request));
  co_return body;
}

Awaitable<ResponseBody> ServerRuntime::HandleActivateSession(
    ConnectionState& connection,
    ActivateSessionRequest request) {
  const auto response = co_await session_manager_.ActivateSession(request);
  if (!response.status)
    co_return ResponseBody{response};

  std::shared_ptr<ServerSession> session;
  if (response.resumed) {
    auto* attached_session = FindSession(request.authentication_token);
    session =
        attached_session ? sessions_.at(request.authentication_token) : nullptr;
    if (!session) {
      co_return ResponseBody{ActivateSessionResponse{
          .status = scada::StatusCode::Bad_SessionIsLoggedOff}};
    }
  } else {
    session = std::make_shared<ServerSession>(ServerSessionContext{
        .session_id = request.session_id,
        .authentication_token = request.authentication_token,
        .service_context = response.service_context,
        .monitored_item_service = monitored_item_service_,
        .now = now_,
    });
    sessions_[request.authentication_token] = session;
  }

  connection.authentication_token = request.authentication_token;
  co_return ResponseBody{response};
}

}  // namespace opcua
