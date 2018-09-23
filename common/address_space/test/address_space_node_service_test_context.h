#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "base/logger.h"
#include "base/strings/utf_string_conversions.h"
#include "common/address_space/address_space_node_service.h"
#include "common/mock_node_observer.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "core/test/test_address_space.h"

namespace testing {

struct AddressSpaceNodeServiceTestContext {
  AddressSpaceNodeServiceTestContext() {
    // Create custom types, since we don't transfer types.
    client_address_space.AddStaticNode(std::make_unique<scada::ObjectType>(
        server_address_space.kTestTypeId, "TestType",
        base::WideToUTF16(L"TestType")));
    DeclareProperty(server_address_space.kTestTypeId,
                    server_address_space.kTestProp1Id, "TestProp1",
                    base::WideToUTF16(L"TestProp1DisplayName"),
                    scada::id::String);
    DeclareProperty(server_address_space.kTestTypeId,
                    server_address_space.kTestProp2Id, "TestProp2",
                    base::WideToUTF16(L"TestProp2DisplayName"),
                    scada::id::String);
    client_address_space.AddStaticNode(std::make_unique<scada::ReferenceType>(
        server_address_space.kTestRefTypeId, "TestRef",
        base::WideToUTF16(L"TestRef")));

    node_service.Subscribe(node_observer);
    node_service.OnChannelOpened();
  }

  ~AddressSpaceNodeServiceTestContext() {
    node_service.Unsubscribe(node_observer);
  }

  void DeclareProperty(const scada::NodeId& type_definition_id,
                       const scada::NodeId& prop_decl_id,
                       const scada::QualifiedName& browse_name,
                       const scada::LocalizedText& display_name,
                       const scada::NodeId& data_type_id) {
    auto* data_type =
        scada::AsDataType(client_address_space.GetNode(data_type_id));
    assert(data_type);

    client_address_space.AddStaticNode(std::make_unique<scada::GenericVariable>(
        prop_decl_id, std::move(browse_name), std::move(display_name),
        *data_type));

    scada::AddReference(client_address_space, scada::id::HasTypeDefinition,
                        prop_decl_id, scada::id::PropertyType);

    scada::AddReference(client_address_space, scada::id::HasProperty,
                        type_definition_id, prop_decl_id);
  }

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();

  TestAddressSpace server_address_space;

  AddressSpaceImpl2 client_address_space{logger};

  GenericNodeFactory node_factory{client_address_space};

  scada::MockMonitoredItemService monitored_item_service;
  scada::MockMethodService method_service;

  AddressSpaceNodeService node_service{AddressSpaceNodeServiceContext{
      logger, server_address_space, server_address_space, client_address_space,
      node_factory, monitored_item_service, method_service}};

  NiceMock<MockNodeObserver> node_observer;
};

}  // namespace testing
