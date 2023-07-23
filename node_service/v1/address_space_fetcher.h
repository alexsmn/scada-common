#pragma once

#include "scada/node_id.h"
#include "scada/status.h"
#include "node_service/node_fetch_status.h"

namespace v1 {

class AddressSpaceFetcher {
 public:
  virtual ~AddressSpaceFetcher() = default;

  virtual void OnChannelOpened() = 0;
  virtual void OnChannelClosed() = 0;

  virtual std::pair<scada::Status, NodeFetchStatus> GetNodeFetchStatus(
      const scada::NodeId& node_id) const = 0;

  virtual void FetchNode(const scada::NodeId& node_id,
                         const NodeFetchStatus& requested_status) = 0;

  virtual size_t GetPendingTaskCount() const = 0;
};

}  // namespace v1
