#pragma once

#include "core/attribute_service.h"

#include <gmock/gmock.h>

namespace scada {

class MockAttributeService : public AttributeService {
 public:
  MOCK_METHOD2(Read,
               void(const std::vector<ReadValueId>& read_value_ids,
                    const ReadCallback& callback));

  MOCK_METHOD3(Write,
               void(const WriteValue& value,
                    const NodeId& user_id,
                    const StatusCallback& callback));
};

class SimpleMockAttributeService : public AttributeService {
 public:
  MOCK_METHOD1(Read, DataValue(const ReadValueId& value_id));

  MOCK_METHOD3(Write,
               void(const WriteValue& value,
                    const NodeId& user_id,
                    const StatusCallback& callback));

  virtual void Read(const std::vector<ReadValueId>& value_ids,
                    const ReadCallback& callback) final {
    std::vector<DataValue> results(value_ids.size());
    for (size_t i = 0; i < value_ids.size(); ++i)
      results[i] = Read(value_ids[i]);
    callback(StatusCode::Good, std::move(results));
  }
};

}  // namespace scada
