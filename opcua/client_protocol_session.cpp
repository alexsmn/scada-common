#include "opcua/client_protocol_session.h"

#include <utility>
#include <variant>

namespace opcua {

OpcUaClientProtocolSession::OpcUaClientProtocolSession(Context context)
    : connection_{context.connection},
      channel_{context.channel} {}

template <typename Response>
Awaitable<scada::StatusOr<Response>>
OpcUaClientProtocolSession::CallTyped(OpcUaRequestBody request) {
  const std::uint32_t request_handle = channel_.NextRequestHandle();
  auto result =
      co_await channel_.Call(request_handle, std::move(request));
  if (!result.ok()) {
    co_return scada::StatusOr<Response>{result.status()};
  }
  if (auto* fault = std::get_if<OpcUaServiceFault>(&result.value())) {
    co_return scada::StatusOr<Response>{fault->status};
  }
  if (auto* typed = std::get_if<Response>(&result.value())) {
    co_return scada::StatusOr<Response>{std::move(*typed)};
  }
  co_return scada::StatusOr<Response>{scada::Status{scada::StatusCode::Bad}};
}

Awaitable<scada::Status> OpcUaClientProtocolSession::Create(
    base::TimeDelta requested_timeout,
    Identity identity) {
  auto open_status = co_await connection_.Open();
  if (open_status.bad()) {
    co_return open_status;
  }

  // CreateSession: pre-authentication, so channel's authentication_token is
  // empty (already the default).
  auto create_result = co_await CallTyped<OpcUaCreateSessionResponse>(
      OpcUaRequestBody{
          OpcUaCreateSessionRequest{.requested_timeout = requested_timeout}});
  if (!create_result.ok()) {
    co_return create_result.status();
  }
  if (create_result->status.bad()) {
    co_return create_result->status;
  }
  session_id_ = create_result->session_id;
  authentication_token_ = create_result->authentication_token;

  // Subsequent requests (including ActivateSession) need the session's
  // authentication token in the header.
  channel_.set_authentication_token(authentication_token_);

  auto activate_result = co_await CallTyped<OpcUaActivateSessionResponse>(
      OpcUaRequestBody{OpcUaActivateSessionRequest{
          .session_id = session_id_,
          .authentication_token = authentication_token_,
          .user_name = identity.user_name,
          .password = identity.password,
          .delete_existing = false,
          .allow_anonymous = !identity.user_name.has_value(),
      }});
  if (!activate_result.ok()) {
    co_return activate_result.status();
  }
  if (activate_result->status.bad()) {
    co_return activate_result->status;
  }

  is_active_ = true;
  co_return scada::Status{scada::StatusCode::Good};
}

Awaitable<scada::Status> OpcUaClientProtocolSession::Close() {
  if (is_active_) {
    auto close_result = co_await CallTyped<OpcUaCloseSessionResponse>(
        OpcUaRequestBody{OpcUaCloseSessionRequest{
            .session_id = session_id_,
            .authentication_token = authentication_token_,
        }});
    is_active_ = false;
    // Swallow the close status; the connection shutdown below is more
    // important to run than to report.
    (void)close_result;
  }
  (void)(co_await connection_.Close());
  co_return scada::Status{scada::StatusCode::Good};
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
OpcUaClientProtocolSession::Read(std::vector<scada::ReadValueId> inputs) {
  auto result = co_await CallTyped<ReadResponse>(
      OpcUaRequestBody{ReadRequest{.inputs = std::move(inputs)}});
  if (!result.ok()) {
    co_return scada::StatusOr<std::vector<scada::DataValue>>{result.status()};
  }
  if (result->status.bad()) {
    co_return scada::StatusOr<std::vector<scada::DataValue>>{result->status};
  }
  co_return scada::StatusOr<std::vector<scada::DataValue>>{
      std::move(result->results)};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
OpcUaClientProtocolSession::Write(std::vector<scada::WriteValue> inputs) {
  auto result = co_await CallTyped<WriteResponse>(
      OpcUaRequestBody{WriteRequest{.inputs = std::move(inputs)}});
  if (!result.ok()) {
    co_return scada::StatusOr<std::vector<scada::StatusCode>>{result.status()};
  }
  if (result->status.bad()) {
    co_return scada::StatusOr<std::vector<scada::StatusCode>>{result->status};
  }
  co_return scada::StatusOr<std::vector<scada::StatusCode>>{
      std::move(result->results)};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
OpcUaClientProtocolSession::Browse(std::vector<scada::BrowseDescription> inputs) {
  auto result = co_await CallTyped<BrowseResponse>(OpcUaRequestBody{
      BrowseRequest{.inputs = std::move(inputs)}});
  if (!result.ok()) {
    co_return scada::StatusOr<std::vector<scada::BrowseResult>>{result.status()};
  }
  if (result->status.bad()) {
    co_return scada::StatusOr<std::vector<scada::BrowseResult>>{result->status};
  }
  co_return scada::StatusOr<std::vector<scada::BrowseResult>>{
      std::move(result->results)};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
OpcUaClientProtocolSession::BrowseNext(
    std::vector<scada::ByteString> continuation_points,
    bool release_continuation_points) {
  auto result = co_await CallTyped<BrowseNextResponse>(OpcUaRequestBody{
      BrowseNextRequest{
          .release_continuation_points = release_continuation_points,
          .continuation_points = std::move(continuation_points),
      }});
  if (!result.ok()) {
    co_return scada::StatusOr<std::vector<scada::BrowseResult>>{result.status()};
  }
  if (result->status.bad()) {
    co_return scada::StatusOr<std::vector<scada::BrowseResult>>{result->status};
  }
  co_return scada::StatusOr<std::vector<scada::BrowseResult>>{
      std::move(result->results)};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
OpcUaClientProtocolSession::TranslateBrowsePathsToNodeIds(
    std::vector<scada::BrowsePath> inputs) {
  auto result = co_await CallTyped<TranslateBrowsePathsResponse>(
      OpcUaRequestBody{
          TranslateBrowsePathsRequest{.inputs = std::move(inputs)}});
  if (!result.ok()) {
    co_return scada::StatusOr<std::vector<scada::BrowsePathResult>>{
        result.status()};
  }
  if (result->status.bad()) {
    co_return scada::StatusOr<std::vector<scada::BrowsePathResult>>{
        result->status};
  }
  co_return scada::StatusOr<std::vector<scada::BrowsePathResult>>{
      std::move(result->results)};
}

Awaitable<scada::StatusOr<OpcUaClientProtocolSession::CallResult>>
OpcUaClientProtocolSession::Call(scada::NodeId object_id,
                         scada::NodeId method_id,
                         std::vector<scada::Variant> arguments) {
  auto result = co_await CallTyped<CallResponse>(
      OpcUaRequestBody{CallRequest{.methods = {OpcUaMethodCallRequest{
                                       .object_id = std::move(object_id),
                                       .method_id = std::move(method_id),
                                       .arguments = std::move(arguments),
                                   }}}});
  if (!result.ok()) {
    co_return scada::StatusOr<CallResult>{result.status()};
  }
  if (result->results.empty()) {
    co_return scada::StatusOr<CallResult>{
        scada::Status{scada::StatusCode::Bad}};
  }
  auto& first = result->results.front();
  co_return scada::StatusOr<CallResult>{CallResult{
      .status = first.status,
      .input_argument_results = std::move(first.input_argument_results),
      .output_arguments = std::move(first.output_arguments),
  }};
}

}  // namespace opcua
