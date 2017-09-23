#pragma once

#include <functional>
#include <vector>

#include "core/configuration_types.h"
#include "core/node_id.h"

namespace scada {

class Status;
using PropertyIds = std::vector<NodeId>;

struct BrowseNode {
  NodeId parent_id;
  NodeId reference_type_id;
  NodeId node_id;
  NodeClass node_class;
  NodeId type_id;
  QualifiedName browse_name;
  LocalizedText display_name;
  NodeId data_type_id;
  Variant value;
};

struct ViewReference {
  NodeId reference_type_id;
  NodeId source_id;
  NodeId target_id;
};

struct RelativePathElement {
  NodeId reference_type_id;
  std::string target_name;
};

using RelativePath = std::vector<RelativePathElement>;

class ViewEvents {
 public:
  virtual ~ViewEvents() {}

  // Model
  virtual void OnNodeAdded(const NodeId& node_id) = 0;
  virtual void OnNodeDeleted(const NodeId& node_id) = 0;
  virtual void OnReferenceAdded(const ViewReference& reference) = 0;
  virtual void OnReferenceDeleted(const ViewReference& reference) = 0;

  // Semantics. Node properties modified.
  virtual void OnNodeModified(const NodeId& node_id, const PropertyIds& property_ids) = 0;
};

using BrowseCallback = std::function<void(const Status& status, std::vector<BrowseResult> results)>;
using TranslateBrowsePathCallback = std::function<void(const Status& status,
    const std::vector<NodeId>& target_ids, size_t remaining_path_index)>;

class ViewService {
 public:
  virtual ~ViewService() {}

  virtual void Browse(const std::vector<BrowseDescription>& nodes, const BrowseCallback& callback) = 0;

  virtual void TranslateBrowsePath(const NodeId& starting_node_id, const RelativePath& relative_path,
      const TranslateBrowsePathCallback& callback) = 0;

  virtual void Subscribe(ViewEvents& events) = 0;
  virtual void Unsubscribe(ViewEvents& events) = 0;
};

class LocalViewService {
 public:
  ~LocalViewService() {}

  virtual BrowseResult Browse(const BrowseDescription& node) = 0;
};

} // namespace scada
