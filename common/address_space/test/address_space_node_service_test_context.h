#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "base/logger.h"
#include "common/address_space/address_space_node_service.h"
#include "common/scada_node_ids.h"

struct AddressSpaceNodeServiceTestContext {
  AddressSpaceNodeServiceTestContext() {
    scada::AddReference(address_space, scada::id::HasTypeDefinition, root.id(),
                        scada::id::FolderType);
    scada::AddReference(address_space, scada::id::HasTypeDefinition,
                        data_items.id(), scada::id::FolderType);
  }

  AddressSpaceNodeServiceContext MakeAddressSpaceNodeServiceContext() {
    NodeFetchStatusChecker node_fetch_status_checker =
        [](const scada::NodeId& node_id) { return NodeFetchStatus::Max(); };

    NodeFetchHandler node_fetch_handler =
        [](const scada::NodeId& node_id,
           const NodeFetchStatus& requested_status) {};

    return {logger, node_fetch_status_checker, node_fetch_handler,
            address_space};
  }

  scada::GenericObject& CreateDataItem(
      const scada::NodeId& node_id,
      const scada::LocalizedText& display_name) {
    auto& node = address_space.AddStaticNode<scada::GenericObject>(
        node_id, display_name);
    scada::AddReference(address_space, scada::id::HasTypeDefinition, node_id,
                        id::DataItemType);
    scada::AddReference(address_space, scada::id::Organizes, data_items, node);
    return node;
  }

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();

  AddressSpaceImpl address_space{logger};

  AddressSpaceNodeService node_service{MakeAddressSpaceNodeServiceContext()};

  scada::GenericObject& root =
      address_space.AddStaticNode<scada::GenericObject>(scada::id::RootFolder,
                                                        L"Root");
  scada::GenericObject& data_items =
      address_space.AddStaticNode<scada::GenericObject>(id::DataItems,
                                                        L"DataItems");
  scada::GenericObject& data_item1 =
      CreateDataItem(scada::NodeId{1, 1}, L"DataItem1");
};
