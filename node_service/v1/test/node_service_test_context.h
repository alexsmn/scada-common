#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/test/test_address_space.h"
#include "base/logger.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_executor.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "node_service/mock_node_observer.h"
#include "node_service/v1/node_service_impl.h"

namespace v1 {

struct NodeServiceTestContext {
  NodeServiceTestContext() {
    node_service.Subscribe(node_observer);
    node_service.OnChannelOpened();
  }

  ~NodeServiceTestContext() {
    node_service.Unsubscribe(node_observer);

    client_address_space.Clear();
  }

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();
  const std::shared_ptr<TestExecutor> executor =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space;

  AddressSpaceImpl client_address_space{logger};
  StandardAddressSpace client_standard_address_space{client_address_space};

  GenericNodeFactory node_factory{client_address_space};

  scada::MockMonitoredItemService monitored_item_service;
  scada::MockMethodService method_service;

  scada::ViewEvents* view_events = nullptr;

  ViewEventsProvider view_events_provider = [this](scada::ViewEvents& events)
      -> std::unique_ptr<IViewEventsSubscription> {
    assert(!view_events);
    view_events = &events;
    return std::make_unique<IViewEventsSubscription>();
  };

  NodeServiceImpl node_service{NodeServiceImplContext{
      executor, view_events_provider, server_address_space,
      server_address_space, client_address_space, node_factory,
      monitored_item_service, method_service}};

  testing::NiceMock<MockNodeObserver> node_observer;
};

}  // namespace v1
