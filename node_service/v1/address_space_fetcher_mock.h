#pragma once

#include "node_service/v1/address_space_fetcher.h"

#include <gmock/gmock.h>

namespace v1 {

class MockAddressSpaceFetcher : public AddressSpaceFetcher {
 public:
  MOCK_METHOD(void, OnChannelOpened, (), (override));
  MOCK_METHOD(void, OnChannelClosed, (), (override));

  MOCK_METHOD((std::pair<scada::Status, NodeFetchStatus>),
              GetNodeFetchStatus,
              (const scada::NodeId& node_id),
              (const override));

  MOCK_METHOD(void,
              FetchNode,
              (const scada::NodeId& node_id,
               const NodeFetchStatus& requested_status),
              (override));

  MOCK_METHOD(size_t, GetPendingTaskCount, (), (const override));
};

}  // namespace v1
