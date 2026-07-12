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

// Loads static address-space nodes from a standard OPC UA UANodeSet2 XML file
// (`http://opcfoundation.org/UA/2011/03/UANodeSet.xsd`, OPC UA Part 6). This is
// the standards-based replacement for the repo-owned format above; it
// materializes into the same address space via the same node factory. See
// common/model/docs/uanodeset-migration.md.
Status LoadUANodeSetXml(const std::filesystem::path& path,
                        MutableAddressSpace& address_space,
                        NodeFactory& node_factory);

// Loads multiple UANodeSet2 files in order (resolving cross-file references
// once all nodes are parsed, like LoadStaticAddressSpace).
Status LoadStaticUANodeSet(std::span<const std::filesystem::path> paths,
                           MutableAddressSpace& address_space,
                           NodeFactory& node_factory);

// Loads multiple static nodeset files, auto-detecting each file's format
// (repo-owned scada-node-state-v1 or standard UANodeSet2) from its root element.
// All files materialize together so references resolve across files and formats
// -- the transitional loader for the UANodeSet2 migration.
Status LoadStaticNodesets(std::span<const std::filesystem::path> paths,
                          MutableAddressSpace& address_space,
                          NodeFactory& node_factory);

// Saves all reachable nodes from `address_space` to the static XML format.
Status SaveAddressSpaceXml(const std::filesystem::path& path,
                           const AddressSpace& address_space);

}  // namespace scada
