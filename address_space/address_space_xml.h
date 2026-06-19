#pragma once

#include "scada/status.h"

#include <filesystem>
#include <span>

namespace scada {
class AddressSpace;
}  // namespace scada

class MutableAddressSpace;
class NodeFactory;

namespace scada {

// Loads static address-space nodes from a repo-owned XML representation.
Status LoadAddressSpaceXml(const std::filesystem::path& path,
                           MutableAddressSpace& address_space,
                           NodeFactory& node_factory);

// Loads multiple static address-space XML files in order.
Status LoadStaticAddressSpace(std::span<const std::filesystem::path> paths,
                              MutableAddressSpace& address_space,
                              NodeFactory& node_factory);

// Saves all reachable nodes from `address_space` to the static XML format.
Status SaveAddressSpaceXml(const std::filesystem::path& path,
                           const AddressSpace& address_space);

std::filesystem::path GetScadaStaticAddressSpaceXmlPath();

}  // namespace scada
