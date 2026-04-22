#include "opcua_ws/opcua_ws_runtime.h"

#include "base/boost_log.h"
#include "base/callback_awaitable.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace opcua {

namespace {

BoostLogger logger_{LOG_NAME("OpcUaWsRuntime")};

template <typename Response>
Response SessionMissingResponse() {
  return {.status = scada::StatusCode::Bad_SessionIsLoggedOff};
}

template <>
OpcUaResponseBody SessionMissingResponse<OpcUaResponseBody>() {
  return OpcUaServiceFault{scada::StatusCode::Bad_SessionIsLoggedOff};
}

template <typename Request>
Awaitable<OpcUaServiceResponse> HandleServiceRequest(const OpcUaRuntimeContext& context,
                                                     const OpcUaSession& session,
                                                     Request request) {
  const auto user_id = session.GetServiceContext().user_id();
  co_return co_await OpcUaServiceHandler{
      {.executor = context.executor,
       .attribute_service = context.attribute_service,
       .view_service = context.view_service,
       .history_service = context.history_service,
       .method_service = context.method_service,
       .node_management_service = context.node_management_service,
       .user_id = user_id}}
      .Handle(OpcUaServiceRequest{std::move(request)});
}

}  // namespace

OpcUaRuntime::OpcUaRuntime(OpcUaRuntimeContext&& context)
    : OpcUaRuntimeContext{std::move(context)} {}

void OpcUaRuntime::Detach(OpcUaConnectionState& connection) {
  if (!connection.authentication_token.has_value())
    return;

  LOG_INFO(logger_) << "OPC UA WS runtime detaching connection session"
                    << LOG_TAG("AuthenticationToken",
                               connection.authentication_token->ToString());
  this->session_manager.DetachSession(*connection.authentication_token);
  connection.authentication_token.reset();
}

OpcUaSession* OpcUaRuntime::FindSession(
    const scada::NodeId& authentication_token) const {
  const auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? it->second.get() : nullptr;
}

OpcUaSession* OpcUaRuntime::FindAttachedSession(
    const OpcUaConnectionState& connection) const {
  if (!connection.authentication_token.has_value())
    return nullptr;
  return FindSession(*connection.authentication_token);
}

void OpcUaRuntime::ForgetSession(const scada::NodeId& authentication_token) {
  LOG_INFO(logger_) << "OPC UA WS runtime forgetting session state"
                    << LOG_TAG("AuthenticationToken",
                               authentication_token.ToString());
  RemoveSessionSubscriptions(authentication_token);
  sessions_.erase(authentication_token);
}

void OpcUaRuntime::IndexSessionSubscriptions(
    const scada::NodeId& authentication_token,
    const OpcUaSession& session) {
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

void OpcUaRuntime::RemoveSessionSubscriptions(
    const scada::NodeId& authentication_token) {
  std::erase_if(subscription_owners_,
                [&](const auto& entry) {
                  return entry.second == authentication_token;
                });
}

Awaitable<void> OpcUaRuntime::Delay(base::TimeDelta delay) const {
  if (delay <= base::TimeDelta{})
    co_return;

  co_await CallbackToAwaitable<>(
      this->executor, [executor = this->executor, delay](auto done) mutable {
        executor->PostDelayedTask(
            std::chrono::milliseconds{delay.InMilliseconds()},
            [done = std::move(done)]() mutable { done(); });
      });
}

Awaitable<OpcUaResponseBody> OpcUaRuntime::Handle(
    OpcUaConnectionState& connection,
    OpcUaRequestBody request) {
  auto body = co_await std::visit(
      [this, &connection](auto&& typed_request) -> Awaitable<OpcUaResponseBody> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaCreateSessionRequest>) {
          co_return OpcUaResponseBody{
              co_await this->session_manager.CreateSession(std::move(typed_request))};
        } else if constexpr (std::is_same_v<T, OpcUaActivateSessionRequest>) {
          co_return co_await HandleActivateSession(connection,
                                                   std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaCloseSessionRequest>) {
          const auto response = this->session_manager.CloseSession(typed_request);
          if (response.status)
            ForgetSession(typed_request.authentication_token);
          if (connection.authentication_token == typed_request.authentication_token)
            connection.authentication_token.reset();
          co_return OpcUaResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaCreateSubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaCreateSubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->CreateSubscriptionWithId(
              next_subscription_id_++, typed_request);
          subscription_owners_[response.subscription_id] =
              *connection.authentication_token;
          co_return OpcUaResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaModifySubscriptionRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaModifySubscriptionResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->ModifySubscription(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaSetPublishingModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaSetPublishingModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->SetPublishingMode(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaDeleteSubscriptionsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaDeleteSubscriptionsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          const auto response = session->DeleteSubscriptions(typed_request);
          for (size_t i = 0; i < typed_request.subscription_ids.size(); ++i) {
            if (response.results[i] == scada::StatusCode::Good)
              subscription_owners_.erase(typed_request.subscription_ids[i]);
          }
          co_return OpcUaResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaPublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaPublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          auto ack_results = session->AcknowledgePublishRequest(typed_request);
          for (;;) {
            if (connection.closed) {
              co_return OpcUaResponseBody{
                  SessionMissingResponse<OpcUaPublishResponse>()};
            }

            session = FindAttachedSession(connection);
            if (!session) {
              co_return OpcUaResponseBody{
                  SessionMissingResponse<OpcUaPublishResponse>()};
            }

            // cppcheck-suppress nullPointerRedundantCheck
            auto poll = session->PollPublish();
            if (poll.response.has_value()) {
              poll.response->results = std::move(ack_results);
              co_return OpcUaResponseBody{std::move(*poll.response)};
            }
            if (!poll.wait_for.has_value()) {
              co_return OpcUaResponseBody{
                  OpcUaPublishResponse{.status = scada::StatusCode::Good,
                                       .results = std::move(ack_results)}};
            }
            co_await Delay(*poll.wait_for);
          }
        } else if constexpr (std::is_same_v<T, OpcUaRepublishRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaRepublishResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->Republish(typed_request)};
        } else if constexpr (std::is_same_v<T,
                                            OpcUaTransferSubscriptionsRequest>) {
          auto* target_session = FindAttachedSession(connection);
          if (!target_session || !connection.authentication_token.has_value()) {
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaTransferSubscriptionsResponse>()};
          }

          OpcUaTransferSubscriptionsResponse response{
              .status = scada::StatusCode::Good};
          response.results.assign(typed_request.subscription_ids.size(),
                                  scada::StatusCode::Bad_WrongSubscriptionId);
          std::unordered_map<scada::NodeId,
                             std::vector<std::pair<size_t, OpcUaSubscriptionId>>>
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

            OpcUaTransferSubscriptionsRequest grouped_request;
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

          co_return OpcUaResponseBody{response};
        } else if constexpr (std::is_same_v<T, OpcUaCreateMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaCreateMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->CreateMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaModifyMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaModifyMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->ModifyMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaDeleteMonitoredItemsRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaDeleteMonitoredItemsResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->DeleteMonitoredItems(typed_request)};
        } else if constexpr (std::is_same_v<T, OpcUaSetMonitoringModeRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return OpcUaResponseBody{
                SessionMissingResponse<OpcUaSetMonitoringModeResponse>()};
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->SetMonitoringMode(typed_request)};
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto response = co_await HandleServiceRequest(*this, *session,
                                                        BrowseRequest{
                                                            .inputs = std::move(
                                                                typed_request.inputs),
                                                        });
          auto* browse_response = std::get_if<BrowseResponse>(&response);
          if (!browse_response)
            co_return SessionMissingResponse<OpcUaResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          auto paged_response = session->StoreBrowseResults(
              std::move(*browse_response),
              typed_request.requested_max_references_per_node);
          co_return OpcUaResponseBody{std::move(paged_response)};
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaResponseBody>();
          // cppcheck-suppress nullPointerRedundantCheck
          co_return OpcUaResponseBody{session->BrowseNext(typed_request)};
        } else {
          auto* session = FindAttachedSession(connection);
          if (!session)
            co_return SessionMissingResponse<OpcUaResponseBody>();

          // cppcheck-suppress nullPointerRedundantCheck
          auto response =
              co_await HandleServiceRequest(*this, *session, std::move(typed_request));
          co_return std::visit(
              [](auto&& typed_response) -> OpcUaResponseBody {
                return OpcUaResponseBody{std::move(typed_response)};
              },
              std::move(response));
        }
      },
      std::move(request));
  co_return body;
}

Awaitable<OpcUaResponseBody> OpcUaRuntime::HandleActivateSession(
    OpcUaConnectionState& connection,
    OpcUaActivateSessionRequest request) {
  const auto response = co_await this->session_manager.ActivateSession(request);
  if (!response.status)
    co_return OpcUaResponseBody{response};

  std::shared_ptr<OpcUaSession> session;
  if (response.resumed) {
    auto* attached_session = FindSession(request.authentication_token);
    session = attached_session ? sessions_.at(request.authentication_token)
                               : nullptr;
    if (!session) {
      co_return OpcUaResponseBody{OpcUaActivateSessionResponse{
          .status = scada::StatusCode::Bad_SessionIsLoggedOff}};
    }
  } else {
    session = std::make_shared<OpcUaSession>(OpcUaSessionContext{
        .session_id = request.session_id,
        .authentication_token = request.authentication_token,
        .service_context = response.service_context,
        .monitored_item_service = this->monitored_item_service,
        .now = this->now,
    });
    sessions_[request.authentication_token] = session;
  }

  connection.authentication_token = request.authentication_token;
  co_return OpcUaResponseBody{response};
}

}  // namespace opcua
