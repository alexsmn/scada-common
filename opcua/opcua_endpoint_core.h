#pragma once

#include "scada/attribute_service.h"
#include "scada/service_context.h"
#include "scada/view_service.h"

#include <memory>

namespace scada::opcua_endpoint {

inline ServiceContext MakeServiceContext(const scada::NodeId& user_id,
                                         ServiceContext base_context = {}) {
  return base_context.with_user_id(user_id);
}

inline scada::DataValue NormalizeReadResult(scada::DataValue result) {
  constexpr unsigned kBadNodeIdUnknownFullCode = 0x80340000u;
  if (result.status_code == scada::StatusCode::Bad_WrongNodeId) {
    result.status_code =
        scada::Status::FromFullCode(kBadNodeIdUnknownFullCode).code();
  }
  return result;
}

inline std::vector<scada::DataValue> NormalizeReadResults(
    std::vector<scada::DataValue> results) {
  for (auto& result : results)
    result = NormalizeReadResult(std::move(result));
  return results;
}

inline void Read(scada::AttributeService& attribute_service,
                 ServiceContext context,
                 std::shared_ptr<const std::vector<scada::ReadValueId>> inputs,
                 const scada::ReadCallback& callback) {
  attribute_service.Read(
      context, std::move(inputs),
      [callback](scada::Status status, std::vector<scada::DataValue> results) {
        callback(std::move(status), NormalizeReadResults(std::move(results)));
      });
}

inline void Write(scada::AttributeService& attribute_service,
                  ServiceContext context,
                  std::shared_ptr<const std::vector<scada::WriteValue>> inputs,
                  const scada::WriteCallback& callback) {
  attribute_service.Write(context, std::move(inputs), callback);
}

inline void Browse(scada::ViewService& view_service,
                   ServiceContext context,
                   std::vector<scada::BrowseDescription> inputs,
                   const scada::BrowseCallback& callback) {
  view_service.Browse(context, inputs, callback);
}

inline void TranslateBrowsePaths(
    scada::ViewService& view_service,
    std::vector<scada::BrowsePath> inputs,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service.TranslateBrowsePaths(inputs, callback);
}

}  // namespace scada::opcua_endpoint
