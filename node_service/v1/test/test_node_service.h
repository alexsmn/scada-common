#pragma once

#include "core/attribute_service_mock.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "node_service/v1/address_space_fetcher_factory.h"
#include "node_service/v1/address_space_fetcher_mock.h"
#include "node_service/v1/node_service_impl.h"

#include <gmock/gmock.h>

namespace v1 {

inline std::shared_ptr<NodeService> CreateTestNodeService(
    scada::AddressSpace& address_space) {
  using namespace testing;

  struct Holder {
    explicit Holder(scada::AddressSpace& address_space)
        : address_space{address_space} {}

    AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory() {
      return [](AddressSpaceFetcherFactoryContext&& context) {
        auto address_space_fetcher =
            std::make_shared<NiceMock<MockAddressSpaceFetcher>>();
        ON_CALL(*address_space_fetcher, GetNodeFetchStatus(_))
            .WillByDefault(
                Return(std::make_pair(scada::Status{scada::StatusCode::Good},
                                      NodeFetchStatus::Max())));
        return address_space_fetcher;
      };
    }

    scada::AddressSpace& address_space;

    NiceMock<scada::MockAttributeService> attribute_service;
    NiceMock<scada::MockMonitoredItemService> monitored_item_service;
    NiceMock<scada::MockMethodService> method_service;

    NodeServiceImpl node_service{NodeServiceImplContext{
        MakeAddressSpaceFetcherFactory(), address_space, attribute_service,
        monitored_item_service, method_service}};
  };

  auto holder = std::make_shared<Holder>(address_space);
  return std::shared_ptr<NodeService>(holder, &holder->node_service);
}

}  // namespace v1
