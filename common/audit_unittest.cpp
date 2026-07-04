#include "common/audit.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "metrics/tracer.h"
#include "scada/attribute_service_mock.h"
#include "scada/test/status_matchers.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

namespace {

using namespace std::chrono_literals;
using testing::_;
using testing::StrictMock;

class TestCoroutineAuditServices final
    : public scada::AttributeService,
      public scada::ViewService {
 public:
  Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::vector<scada::ReadValueId> inputs) override {
    ++read_count;
    last_read_inputs = std::move(inputs);
    co_return read_results;
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::vector<scada::WriteValue> inputs) override {
    ++write_count;
    co_return std::vector<scada::StatusCode>(inputs.size(),
                                             scada::StatusCode::Good);
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> inputs) override {
    ++browse_count;
    last_browse_inputs = std::move(inputs);
    co_return browse_results;
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override {
    ++translate_count;
    co_return std::vector<scada::BrowsePathResult>(inputs.size());
  }

  int read_count = 0;
  int write_count = 0;
  int browse_count = 0;
  int translate_count = 0;
  std::vector<scada::ReadValueId> last_read_inputs;
  std::vector<scada::BrowseDescription> last_browse_inputs;
  std::vector<scada::DataValue> read_results{{}};
  std::vector<scada::BrowseResult> browse_results{{}};
};

DataServices MakeAuditDataServices(const scada::services& services) {
  return DataServices::FromUnownedServices(services);
}

TEST(AuditTest, CoroutineReadRecordsMetric) {
  TestExecutor executor;
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();
  scada::services services{.attribute_service = attribute_service.get()};
  auto audit = Audit::Create(
      AuditContext{.data_services_ = MakeAuditDataServices(services),
                   .tracer_ = Tracer::None(),
                   .executor_ = executor});

  EXPECT_CALL(*attribute_service, Read(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::ReadValueId>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::DataValue>>> {
        co_return std::vector<scada::DataValue>{scada::DataValue{}};
      });

  auto read_result = WaitAwaitable(
      executor, audit->Read({}, std::vector<scada::ReadValueId>{}));

  ASSERT_THAT(read_result, scada::test::IsOkAndHolds(testing::SizeIs(1)));
}

TEST(AuditTest, CoroutineBrowseUsesViewServiceAndRecordsMetric) {
  TestExecutor executor;
  auto view_service = std::make_shared<StrictMock<scada::MockViewService>>();
  scada::services services{.view_service = view_service.get()};
  auto audit = Audit::Create(
      AuditContext{.data_services_ = MakeAuditDataServices(services),
                   .tracer_ = Tracer::None(),
                   .executor_ = executor});

  EXPECT_CALL(*view_service, Browse(_, _))
      .WillOnce([](scada::ServiceContext,
                   std::vector<scada::BrowseDescription>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::BrowseResult>>> {
        co_return std::vector<scada::BrowseResult>{};
      });

  auto result = StartAwaitable(
      executor, static_cast<scada::ViewService&>(*audit).Browse(
                    {}, {scada::BrowseDescription{}}));
  Drain(executor);

  auto browse_result = WaitResult(executor, result);
  ASSERT_THAT(browse_result, scada::test::IsOkAndHolds(testing::IsEmpty()));
}

TEST(AuditTest, AuditScadaServicesWithExecutorWrapsAttributes) {
  TestExecutor executor;
  StrictMock<scada::MockAttributeService> attribute_service;
  auto source_services = std::make_shared<scada::services>(
      scada::services{.attribute_service = &attribute_service});
  auto audited_services =
      AuditScadaServices(source_services, Tracer::None(), executor);

  ASSERT_NE(audited_services->attribute_service, nullptr);
  EXPECT_NE(audited_services->attribute_service, &attribute_service);

  EXPECT_CALL(attribute_service, Read(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::ReadValueId>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::DataValue>>> {
        co_return std::vector<scada::DataValue>{};
      });

  auto read_result = WaitAwaitable(
      executor, audited_services->attribute_service->Read(
                    {}, std::vector<scada::ReadValueId>{}));

  ASSERT_THAT(read_result, scada::test::IsOkAndHolds(testing::IsEmpty()));
}

TEST(AuditTest, AuditDataServicesWrapsDirectCoroutineSlots) {
  TestExecutor executor;
  auto source_services = std::make_shared<TestCoroutineAuditServices>();

  DataServices data_services;
  data_services.attribute_service_ = source_services;
  data_services.view_service_ = source_services;

  auto audited_services =
      AuditDataServices(std::move(data_services), Tracer::None(), executor);

  ASSERT_NE(audited_services->attribute_service_, nullptr);
  ASSERT_NE(audited_services->attribute_service_, nullptr);
  ASSERT_NE(audited_services->view_service_, nullptr);
  ASSERT_NE(audited_services->view_service_, nullptr);
  EXPECT_NE(audited_services->attribute_service_.get(),
            source_services.get());
  EXPECT_NE(audited_services->view_service_.get(),
            source_services.get());

  const std::vector<scada::ReadValueId> read_inputs{
      {.node_id = scada::NodeId{1}}};
  auto read_result = WaitAwaitable(
      executor, audited_services->attribute_service_->Read({}, read_inputs));

  ASSERT_THAT(read_result, scada::test::IsOkAndHolds(testing::Eq(
                               source_services->read_results)));
  EXPECT_EQ(source_services->read_count, 1);
  EXPECT_EQ(source_services->last_read_inputs, read_inputs);

  const std::vector<scada::WriteValue> write_inputs{
      {.node_id = scada::NodeId{4}}};
  auto write_result = WaitAwaitable(
      executor, audited_services->attribute_service_->Write({}, write_inputs));

  ASSERT_THAT(write_result, scada::test::IsOkAndHolds(testing::ElementsAre(
                                scada::StatusCode::Good)));
  EXPECT_EQ(source_services->write_count, 1);

  auto browse_result =
      WaitAwaitable(executor, audited_services->view_service_->Browse(
                                  {}, {{.node_id = scada::NodeId{2}}}));

  ASSERT_THAT(browse_result, scada::test::IsOkAndHolds(testing::Eq(
                                 source_services->browse_results)));
  EXPECT_EQ(source_services->browse_count, 1);
  ASSERT_EQ(source_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(source_services->last_browse_inputs[0].node_id, (scada::NodeId{2}));

  browse_result =
      WaitAwaitable(executor, audited_services->view_service_->Browse(
                                  {}, {{.node_id = scada::NodeId{3}}}));

  ASSERT_THAT(browse_result, scada::test::IsOkAndHolds(testing::Eq(
                                 source_services->browse_results)));
  EXPECT_EQ(source_services->browse_count, 2);
  ASSERT_EQ(source_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(source_services->last_browse_inputs[0].node_id, (scada::NodeId{3}));

  auto translate_result = WaitAwaitable(
      executor, audited_services->view_service_->TranslateBrowsePaths(
                    {scada::BrowsePath{}}));

  ASSERT_THAT(translate_result,
              scada::test::IsOkAndHolds(testing::SizeIs(1)));
  EXPECT_EQ(source_services->translate_count, 1);
}

TEST(AuditTest, AuditDataServicesWrapsUnownedServicesForCoroutineUse) {
  TestExecutor executor;
  StrictMock<scada::MockAttributeService> attribute_service;

  scada::services source_services{.attribute_service = &attribute_service};
  auto audited_services =
      AuditDataServices(MakeAuditDataServices(source_services), Tracer::None(),
                        executor);

  ASSERT_NE(audited_services->attribute_service_, nullptr);
  ASSERT_NE(audited_services->attribute_service_, nullptr);
  EXPECT_NE(audited_services->attribute_service_.get(), &attribute_service);

  EXPECT_CALL(attribute_service, Read(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::ReadValueId>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::DataValue>>> {
        co_return std::vector<scada::DataValue>{scada::DataValue{}};
      });

  const std::vector<scada::ReadValueId> read_inputs{
      {.node_id = scada::NodeId{3}}};
  auto values = WaitAwaitable(
      executor, audited_services->attribute_service_->Read({}, read_inputs));
  ASSERT_THAT(values, scada::test::IsOkAndHolds(testing::SizeIs(1)));
}

}  // namespace
