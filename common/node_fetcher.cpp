#include "node_fetcher.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

NodeRef NodeFetcher::FetchNode(const scada::NodeId& node_id) {
  auto i = fetched_nodes_.find(node_id);
  if (i != fetched_nodes_.end())
    return i->second;

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  node_service_.RequestNode(node_id, [weak_ptr, this](const scada::Status& status, const NodeRef& node) {
    if (!weak_ptr.get())
      return;
    fetched_nodes_.emplace(node.id(), node);
    FOR_EACH_OBSERVER(Observer, observers_, OnNodeFetched(node));
  });

  return nullptr;
}

std::string FetchNodeName(NodeFetcher& fetcher, const scada::NodeId& node_id) {
  if (auto node = fetcher.FetchNode(node_id))
    return node.browse_name();
  else
    return node_id.ToString();
}

base::string16 FetchNodeTitle(NodeFetcher& fetcher, const scada::NodeId& node_id) {
  if (auto node = fetcher.FetchNode(node_id))
    return node.display_name();
  else
    return base::SysNativeMBToWide(node_id.ToString());
}
