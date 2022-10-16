#pragma once

#include "common/variable_handle.h"

namespace scada {

class Node;
class Variable;

class NodeVariableHandle : public VariableHandle {
 public:
  explicit NodeVariableHandle(Variable& node);

  virtual Node& GetNode();

  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const scada::WriteValue& input,
      const scada::StatusCallback& callback) override;
  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const NodeId& user_id,
                    const StatusCallback& callback) override;

 private:
  Variable& node_;

  DISALLOW_COPY_AND_ASSIGN(NodeVariableHandle);
};

}  // namespace scada
