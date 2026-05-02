#pragma once

#include "scada/attribute_service.h"
#include "scada/status_promise.h"

namespace scada {

// Prefer `scada::client{...}.node(input.node_id).read(input.attribute_id)`.
inline promise<DataValue> Read(AttributeService& attribute_service,
                               const ServiceContext& context,
                               ReadValueId&& input) {
  promise<DataValue> p;
  Read(attribute_service, context, std::move(input),
       MakeStatusCodePromiseCallback(p));
  return p;
}

inline promise<std::vector<StatusCode>> Write(
    AttributeService& attribute_service,
    const ServiceContext& context,
    std::vector<WriteValue> inputs) {
  promise<std::vector<StatusCode>> p;
  attribute_service.Write(
      context, std::make_shared<std::vector<WriteValue>>(std::move(inputs)),
      [p](Status status, std::vector<StatusCode> results) mutable {
        if (status) {
          p.resolve(std::move(results));
        } else {
          RejectStatusPromise(p, std::move(status));
        }
      });
  return p;
}

inline promise<void> Write(AttributeService& attribute_service,
                           const ServiceContext& context,
                           WriteValue&& input) {
  promise<void> p;
  Write(attribute_service, context, std::move(input),
        MakeStatusPromiseCallback(p));
  return p;
}

}  // namespace scada