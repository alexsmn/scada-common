#pragma once

#include "base/strings/string_piece.h"

#include <memory>

namespace rt {
class TimedData;
}

namespace scada {
class NodeId;
}

class TimedDataService {
 public:
  virtual ~TimedDataService() {}

  virtual std::shared_ptr<rt::TimedData> GetNodeTimedData(const scada::NodeId& node_id) = 0;
};
