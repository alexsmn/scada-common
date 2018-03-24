#pragma once

namespace scada {

class RemoteNode {
 public:
  virtual ~RemoteNode() {}

  virtual void Read(const std::vector<ReadValueId>& value_ids, const ReadCallback& callback) = 0;
};

} // namespace scada
