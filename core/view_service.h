#pragma once

#include <functional>
#include <vector>

#include "core/configuration_types.h"
#include "core/node_class.h"
#include "core/node_id.h"
#include "core/qualified_name.h"
#include "core/status.h"

namespace scada {

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

struct RelativePathElement {
  NodeId reference_type_id;
  QualifiedName target_name;
};

using RelativePath = std::vector<RelativePathElement>;

class ViewEvents {
 public:
  virtual ~ViewEvents() {}

  // Model
  virtual void OnNodeAdded(const NodeId& node_id) = 0;
  virtual void OnNodeDeleted(const NodeId& node_id) = 0;
  virtual void OnReferenceAdded(const scada::NodeId& node_id) = 0;
  virtual void OnReferenceDeleted(const scada::NodeId& node_id) = 0;

  virtual void OnNodeSemanticsChanged(const NodeId& node_id) = 0;
};

using BrowseCallback = std::function<void(Status&& status, std::vector<BrowseResult>&& results)>;
using TranslateBrowsePathCallback = std::function<void(Status&& status, std::vector<NodeId>&& target_ids,
    size_t remaining_path_index)>;

class ViewService {
 public:
  virtual ~ViewService() {}

  virtual void Browse(const std::vector<BrowseDescription>& descriptions, const BrowseCallback& callback) = 0;

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
