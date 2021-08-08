#pragma once

#include "base/containers/span.h"
#include "base/struct_writer.h"
#include "core/node_id.h"
#include "core/status.h"

#include <functional>

namespace v1 {

struct NodeFetchStatusChangedItem {
  scada::NodeId node_id;
  scada::Status status;
  NodeFetchStatus fetch_status;
};

using NodeFetchStatusChangedHandler =
    std::function<void(base::span<const NodeFetchStatusChangedItem> items)>;

inline bool operator==(const NodeFetchStatusChangedItem& a,
                       const NodeFetchStatusChangedItem& b) {
  return a.node_id == b.node_id && a.status == b.status &&
         a.fetch_status == b.fetch_status;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const NodeFetchStatusChangedItem& item) {
  StructWriter{stream}
      .AddField("node_id", ToString(item.node_id))
      .AddField("status", ToString(item.status))
      .AddField("fetch_status", ToString(item.fetch_status));
  return stream;
}

}  // namespace v1
