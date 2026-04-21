#include "opcua_ws/opcua_ws_runtime.h"

#include "base/callback_awaitable.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace opcua_ws {

namespace {

template <typename Response>
Response SessionMissingResponse() {
  return {.status = scada::StatusCode::Bad_SessionIsLoggedOff};
}

template <>
OpcUaWsResponseBody SessionMissingResponse<OpcUaWsResponseBody>() {
  return OpcUaWsServiceFault{scada::StatusCode::Bad_SessionIsLoggedOff};
}

}  // namespace

OpcUaWsRuntime::OpcUaWsRuntime(OpcUaWsRuntimeContext&& context)
    : OpcUaWsRuntimeContext{std::move(context)} {}

Awaitable<OpcUaWsResponseMessage> OpcUaWsRuntime::Handle(
    OpcUaWsConnectionState& connection,
    OpcUaWsRequestMessage request) {
  auto body = co_await HandleRequestBody(connection, std::move(request.body));
  co_return OpcUaWsResponseMessage{
      .request_handle = request.request_handle, .body = std::move(body)};
}

void OpcUaWsRuntime::Detach(OpcUaWsConnectionState& connection) {
  if (!connection.authentication_token.has_value())
    return;

  this->session_manager.DetachSession(*connection.authentication_token);
  connection.authentication_token.reset();
}

OpcUaWsSession* OpcUaWsRuntime::FindSession(
    const scada::NodeId& authentication_token) const {
  const auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? it->second.get() : nullptr;
}

OpcUaWsSession* OpcUaWsRuntime::FindAttachedSession(
    const OpcUaWsConnectionState& connection) const {
  if (!connection.authentication_token.has_value())
    return nullptr;
  return FindSession(*connection.authentication_token);
}

void OpcUaWsRuntime::ForgetSession(const scada::NodeId& authentication_token) {
  RemoveSessionSubscriptions(authentication_token);
  sessions_.erase(authentication_token);
}

void OpcUaWsRuntime::IndexSessionSubscriptions(
    const scada::NodeId& authentication_token,
    const OpcUaWsSession& session) {
  const auto subscription_ids = session.GetSubscriptionIds();
  for (const auto subscription_id : subscription_ids)
    subscription_owners_[subscription_id] = authentication_token;
  if (!subscription_ids.empty()) {
    next_subscription_id_ =
        std::max(next_subscription_id_,
                 *std::max_element(subscription_ids.begin(),
                                   subscription_ids.end()) +
                     1);
  }
}

void OpcUaWsRuntime::RemoveSessionSubscriptions(
    const scada::NodeId& authentication_token) {
  std::erase_if(subscription_owners_,
                [&](const auto& entry) {
                  return entry.second == authentication_token;
                });
}

Awaitable<void> OpcUaWsRuntime::Delay(base::TimeDelta delay) const {
  if (delay <= base::TimeDelta{})
    co_return;

  co_await CallbackToAwaitable<>(this->executor,
                                 [executor = this->executor, delay](auto done) mutable {
                                   executor->PostDelayedTask(
                                       std::chrono::milliseconds{
                                           delay.InMilliseconds()},
                                       [done = std::move(done)]() mutable {
                                         done();
                                       });
                                 });
}

Awaitable<OpcUaWsResponseBody> OpcUaWsRuntime::HandleRequestBody(
    OpcUaWsConnectionState& connection,
    OpcUaWsRequestBody request) {
  auto body = co_await std::visit(
      [this, &connection](auto&& typed_request) -> Awaitable<OpcUaWsResponseBody> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaWsCreateSessionRequest>) {
          co_return OpcUaWsResponseBody{
              co_await this->session_manager.CreateSession(std::move(typed_request))};
        } else if constexpr (std::is_same_v<T, OpcUaWsActivateSessionRequest>) {
          co_return co_await HandleActivateSession(connection,
                                                   std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaWsCloseSessionRequest>) {
          const auto response = this->session_manager.CloseSession(typed_request);
          if (response.status)
            ForgetSession(typed_request.authentication_token);
          if (connection.authentication_token == typed_request.authentication_token)
            connection.authentication_token.reset();
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateSubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsCreateSubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->CreateSubscriptionWithId(
              next_subscription_id_++, typed_request);
          subscription_owners_[response.subscription_id] =
              *connection.authentication_token;
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsModifySubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsModifySubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->ModifySubscription(typed_request);
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsSetPublishingModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsSetPublishingModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaWsResponseBody{session->SetPublishingMode(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteSubscriptionsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsDeleteSubscriptionsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->DeleteSubscriptions(typed_request);
          for (size_t i = 0; i < typed_request.subscription_ids.size(); ++i) {
            if (response.results[i] == scada::StatusCode::Good)
              subscription_owners_.erase(typed_request.subscription_ids[i]);
          }
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsPublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsPublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          auto& attached_session = *session;
          auto ack_results =
              attached_session.AcknowledgePublishRequest(typed_request);
          for (;;) {
            auto poll = attached_session.PollPublish();
            if (poll.response.has_value()) {
              poll.response->results = std::move(ack_results);
              co_return OpcUaWsResponseBody{std::move(*poll.response)};
            }
            if (!poll.wait_for.has_value()) {
              co_return OpcUaWsResponseBody{
                  OpcUaWsPublishResponse{.status = scada::StatusCode::Good,
                                         .results = std::move(ack_results)}};
            }
            co_await Delay(*poll.wait_for);
          }
        } else if constexpr (std::is_same_v<T, OpcUaWsRepublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsRepublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaWsResponseBody{session->Republish(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaWsTransferSubscriptionsRequest>) {
          auto* target_session = FindAttachedSession(connection);
          if (!target_session || !connection.authentication_token.has_value()) {
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsTransferSubscriptionsResponse>()};
          }

          OpcUaWsTransferSubscriptionsResponse response{
              .status = scada::StatusCode::Good};
          response.results.assign(typed_request.subscription_ids.size(),
                                  scada::StatusCode::Bad_WrongSubscriptionId);
          std::unordered_map<scada::NodeId,
                             std::vector<std::pair<size_t, OpcUaWsSubscriptionId>>>
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
                response.results[index] = scada::StatusCode::Bad_SessionIsLoggedOff;
              continue;
            }

            OpcUaWsTransferSubscriptionsRequest grouped_request;
            grouped_request.send_initial_values = typed_request.send_initial_values;
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

          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsCreateMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->CreateMonitoredItems(typed_request);
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsModifyMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsModifyMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->ModifyMonitoredItems(typed_request);
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsDeleteMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->DeleteMonitoredItems(typed_request);
          co_return OpcUaWsResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaWsSetMonitoringModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaWsResponseBody{
                SessionMissingResponse<OpcUaWsSetMonitoringModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaWsResponseBody{session->SetMonitoringMode(typed_request)};
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaWsResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto& attached_session = *session;

          const auto user_id = attached_session.GetServiceContext().user_id();
          auto response = co_await OpcUaWsServiceHandler{
              {.executor = this->executor,
               .attribute_service = this->attribute_service,
               .view_service = this->view_service,
               .history_service = this->history_service,
               .method_service = this->method_service,
               .node_management_service = this->node_management_service,
               .user_id = user_id}}
                              .Handle(BrowseRequest{
                                  .requested_max_references_per_node = 0,
                                  .inputs = std::move(typed_request.inputs),
                              });
          auto* browse_response = std::get_if<BrowseResponse>(&response);
          if (!browse_response)
            co_return SessionMissingResponse<OpcUaWsResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto typed_browse_response = std::move(*browse_response);
          // cppcheck-suppress nullPointerRedundantCheck
          auto paged_response = attached_session.StoreBrowseResults(
              std::move(typed_browse_response),
              typed_request.requested_max_references_per_node);
          co_return OpcUaWsResponseBody{std::move(paged_response)};
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaWsResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto& attached_session = *session;
          co_return OpcUaWsResponseBody{attached_session.BrowseNext(typed_request)};
        } else {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaWsResponseBody>();

          // cppcheck-suppress nullPointerRedundantCheck
          const auto user_id = session->GetServiceContext().user_id();
          auto response = co_await OpcUaWsServiceHandler{
              {.executor = this->executor,
               .attribute_service = this->attribute_service,
               .view_service = this->view_service,
               .history_service = this->history_service,
               .method_service = this->method_service,
               .node_management_service = this->node_management_service,
               .user_id = user_id}}
                              .Handle(std::move(typed_request));
          co_return std::visit(
              [](auto&& typed_response) -> OpcUaWsResponseBody {
                return OpcUaWsResponseBody{std::move(typed_response)};
              },
              std::move(response));
        }
      },
      std::move(request));
  co_return body;
}

Awaitable<OpcUaWsResponseBody> OpcUaWsRuntime::HandleActivateSession(
    OpcUaWsConnectionState& connection,
    OpcUaWsActivateSessionRequest request) {
  const auto response = co_await this->session_manager.ActivateSession(request);
  if (!response.status)
    co_return OpcUaWsResponseBody{response};

  std::shared_ptr<OpcUaWsSession> session;
  if (response.resumed) {
    auto* attached_session = FindSession(request.authentication_token);
    session = attached_session ? sessions_.at(request.authentication_token) : nullptr;
    if (!session) {
      co_return OpcUaWsResponseBody{OpcUaWsActivateSessionResponse{
          .status = scada::StatusCode::Bad_SessionIsLoggedOff}};
    }
  } else {
    session = std::make_shared<OpcUaWsSession>(OpcUaWsSessionContext{
        .session_id = request.session_id,
        .authentication_token = request.authentication_token,
        .service_context = response.service_context,
        .monitored_item_service = this->monitored_item_service,
        .now = this->now,
    });
    sessions_[request.authentication_token] = session;
  }

  connection.authentication_token = request.authentication_token;
  co_return OpcUaWsResponseBody{response};
}

}  // namespace opcua_ws
