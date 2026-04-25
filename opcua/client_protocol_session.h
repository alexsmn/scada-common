#pragma once

#include "base/awaitable.h"
#include "base/time/time.h"
#include "opcua/client_channel.h"
#include "opcua/client_connection.h"
#include "opcua/message.h"
#include "scada/attribute_service.h"
#include "scada/localized_text.h"
#include "scada/method_service.h"
#include "scada/status.h"
#include "scada/status_or.h"
#include "scada/variant.h"
#include "scada/view_service.h"

#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace opcua {

// Thin typed facade over OpcUaClientChannel. Drives the Session
// lifecycle (CreateSession -> ActivateSession -> ... -> CloseSession) and
// packages each OPC UA service call into an OpcUaRequestBody /
// OpcUaResponseBody round-trip.
//
// All service methods return Awaitable<StatusOr<Result>> so the caller can
// compose them with other coroutines. Errors at any layer (connection,
// codec, service fault, wrong response type) surface as a bad Status.
class OpcUaClientProtocolSession {
 public:
  struct Context {
    OpcUaClientConnection& connection;
    OpcUaClientChannel& channel;
  };

  explicit OpcUaClientProtocolSession(Context context);

  // Runs the connection-establishment dance:
  //   1. connection.Open()
  //   2. CreateSession
  //   3. ActivateSession (with the given identity)
  struct Identity {
    // Empty user_name selects anonymous.
    std::optional<scada::LocalizedText> user_name;
    std::optional<scada::LocalizedText> password;
  };
  [[nodiscard]] Awaitable<scada::Status> Create(
      base::TimeDelta requested_timeout = base::TimeDelta::FromMinutes(10),
      Identity identity = {});

  // CloseSession + connection.Close(), best-effort.
  [[nodiscard]] Awaitable<scada::Status> Close();

  [[nodiscard]] bool is_active() const { return is_active_; }
  [[nodiscard]] const scada::NodeId& session_id() const { return session_id_; }
  [[nodiscard]] const scada::NodeId& authentication_token() const {
    return authentication_token_;
  }

  // -- Typed service helpers. Each one packages the request variant, calls
  // channel_.Call, then narrows the response variant. A bad Status is
  // returned if any step fails or the response type doesn't match.

  [[nodiscard]] Awaitable<
      scada::StatusOr<std::vector<scada::DataValue>>>
  Read(std::vector<scada::ReadValueId> inputs);

  [[nodiscard]] Awaitable<
      scada::StatusOr<std::vector<scada::StatusCode>>>
  Write(std::vector<scada::WriteValue> inputs);

  [[nodiscard]] Awaitable<
      scada::StatusOr<std::vector<scada::BrowseResult>>>
  Browse(std::vector<scada::BrowseDescription> inputs);

  [[nodiscard]] Awaitable<
      scada::StatusOr<std::vector<scada::BrowseResult>>>
  BrowseNext(std::vector<scada::ByteString> continuation_points,
             bool release_continuation_points = false);

  [[nodiscard]] Awaitable<
      scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePathsToNodeIds(std::vector<scada::BrowsePath> inputs);

  struct CallResult {
    scada::Status status{scada::StatusCode::Good};
    std::vector<scada::StatusCode> input_argument_results;
    std::vector<scada::Variant> output_arguments;
  };
  [[nodiscard]] Awaitable<scada::StatusOr<CallResult>> Call(
      scada::NodeId object_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments);

 private:
  // Helper that sends a typed request and extracts the typed response. On a
  // variant mismatch, decode error, or transport error it yields a bad
  // Status. On OpcUaServiceFault the fault status is propagated.
  template <typename Response>
  [[nodiscard]] Awaitable<scada::StatusOr<Response>>
  CallTyped(OpcUaRequestBody request);

  OpcUaClientConnection& connection_;
  OpcUaClientChannel& channel_;

  bool is_active_ = false;
  scada::NodeId session_id_;
  scada::NodeId authentication_token_;
};

}  // namespace opcua
