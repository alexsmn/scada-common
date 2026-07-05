#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/test/test_address_space.h"
#include "base/check.h"
#include "base/test/test_executor.h"
#include "events/view_events_subscription.h"
#include "node_service/test/recording_node_observer.h"
#include "node_service/v1/address_space_fetcher_mock.h"
#include "node_service/v1/node_service_impl.h"

#include <gmock/gmock.h>

namespace v1 {

struct NodeServiceTestContext {
  NodeServiceTestContext() {
    node_observer.Connect(node_service);
    node_service.OnChannelOpened();
  }

  ~NodeServiceTestContext() {
    node_observer.Disconnect();

    client_address_space.Clear();
  }

  TestExecutor executor;

  TestAddressSpace server_address_space;

  AddressSpaceImpl client_address_space;
  StandardAddressSpace client_standard_address_space{client_address_space};

  GenericNodeFactory node_factory{client_address_space};

  scada::ViewEvents* view_events = nullptr;

  ViewEventsProvider view_events_provider = [this](scada::ViewEvents& events)
      -> std::unique_ptr<IViewEventsSubscription> {
    base::Check(!view_events);
    view_events = &events;
    return std::make_unique<IViewEventsSubscription>();
  };

  NodeServiceImpl node_service{NodeServiceImplContext{
      .address_space_fetcher_factory_ = MakeAddressSpaceFetcherFactory(),
      .address_space_ = client_address_space,
      .scada_client_ = {}}};

  RecordingNodeObserver node_observer;

 private:
  AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory() {
    return [](AddressSpaceFetcherFactoryContext&& context) {
      auto address_space_fetcher =
          std::make_shared<testing::NiceMock<MockAddressSpaceFetcher>>();
      ON_CALL(*address_space_fetcher, GetNodeFetchStatus(testing::_))
          .WillByDefault(testing::Return(std::make_pair(
              scada::Status{scada::StatusCode::Good}, NodeFetchStatus::Max())));
      return address_space_fetcher;
    };
  }
};

}  // namespace v1
