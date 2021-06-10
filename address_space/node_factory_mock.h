#include "address_space/node_factory.h"

#include <gmock/gmock.h>

class MockNodeFactory : public NodeFactory {
 public:
  MOCK_METHOD((std::pair<scada::Status, scada::Node*>),
              CreateNode,
              (const scada::NodeState& node_state),
              (override));
};
