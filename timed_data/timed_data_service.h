#pragma once

#include "base/strings/string_piece.h"
#include "core/aggregation.h"

#include <memory>

namespace rt {
class TimedData;
}

class TimedDataService {
 public:
  virtual ~TimedDataService() {}

  virtual std::shared_ptr<rt::TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::Aggregation& aggregation) = 0;

  virtual std::shared_ptr<rt::TimedData> GetFormulaTimedData(
      base::StringPiece formula,
      const scada::Aggregation& aggregation) = 0;
};
