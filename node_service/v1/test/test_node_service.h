#pragma once

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

    NodeServiceImpl node_service{NodeServiceImplContext{
        .address_space_fetcher_factory_ = MakeAddressSpaceFetcherFactory(),
        .address_space_ = address_space,
        .scada_client_ = {}}};
  };

  auto holder = std::make_shared<Holder>(address_space);
  return std::shared_ptr<NodeService>(holder, &holder->node_service);
}

}  // namespace v1
