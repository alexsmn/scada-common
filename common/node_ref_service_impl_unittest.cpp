#include "node_ref_service_impl.h"

#include "base/logger.h"
#include "core/attribute_service.h"
#include "core/test/test_view_service.h"

#include <gmock/gmock.h>

class TestAttributeService : public scada::AttributeService {
 public:
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids, const scada::ReadCallback& callback) override {
    callback(scada::StatusCode::Bad, {});
  }

  virtual void Write(const scada::NodeId& node_id, double value, const scada::NodeId& user_id,
                     const scada::WriteFlags& flags, const scada::StatusCallback& callback) override {
    callback(scada::StatusCode::Bad);
  }
};

TEST(NodeRefServiceImpl, RequestNodeWithNoDependencies) {
  const auto logger = std::make_shared<NullLogger>();
  test::TestViewService view_service;
  TestAttributeService attribute_service;
  NodeRefServiceImpl service{NodeRefServiceImplContext{
      logger,
      view_service,
      attribute_service,
  }};
}
