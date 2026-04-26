#include "common/audit.h"

#include "base/awaitable_promise.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "metrics/metric_service.h"
#include "metrics/metrics.h"
#include "metrics/tracer.h"
#include "scada/attribute_service_mock.h"
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

class CapturingMetricService final : public MetricService {
 public:
  void RegisterProvider(const Provider& provider) override {
    provider_ = provider;
  }

  void RegisterSink(const Sink& sink) override { sink_ = sink; }

  Metrics Collect(const std::shared_ptr<TestExecutor>& executor) {
    return WaitPromise(executor, provider_());
  }

  Provider provider_;
  Sink sink_;
};

bool HasMetric(const Metrics& metrics, const std::string& name) {
  return metrics.ToUnorderedMap().contains(name);
}

TEST(AuditTest, CallbackReadUsesCoroutineAdapterAndRecordsMetric) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();
  scada::services services{.attribute_service = attribute_service.get()};
  auto audit = Audit::Create(AuditContext{
      .metric_service_ = metric_service,
      .services_ = services,
      .tracer_ = Tracer::None(),
      .executor_ = MakeTestAnyExecutor(executor)});

  scada::ReadCallback pending_read;
  EXPECT_CALL(*attribute_service, Read(_, _, _))
      .WillOnce([&](const scada::ServiceContext&,
                    const std::shared_ptr<const std::vector<scada::ReadValueId>>&,
                    const scada::ReadCallback& callback) {
        pending_read = callback;
      });

  bool read_called = false;
  audit->Read({}, std::make_shared<const std::vector<scada::ReadValueId>>(),
              [&](scada::Status status,
                  std::vector<scada::DataValue> results) {
                read_called = true;
                EXPECT_TRUE(status.good());
                EXPECT_EQ(results.size(), 1u);
              });
  Drain(executor);

  EXPECT_FALSE(read_called);
  ASSERT_TRUE(pending_read);

  pending_read(scada::StatusCode::Good, {scada::DataValue{}});
  Drain(executor);

  EXPECT_TRUE(read_called);
  EXPECT_TRUE(HasMetric(metric_service.Collect(executor), "read_latency.count"));
}

TEST(AuditTest, CoroutineBrowseUsesCallbackAdapterAndRecordsMetric) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  auto view_service = std::make_shared<StrictMock<scada::MockViewService>>();
  scada::services services{.view_service = view_service.get()};
  auto audit = Audit::Create(AuditContext{
      .metric_service_ = metric_service,
      .services_ = services,
      .tracer_ = Tracer::None(),
      .executor_ = MakeTestAnyExecutor(executor)});

  scada::BrowseCallback pending_browse;
  EXPECT_CALL(*view_service, Browse(_, _, _))
      .WillOnce([&](const scada::ServiceContext&,
                    const std::vector<scada::BrowseDescription>&,
                    const scada::BrowseCallback& callback) {
        pending_browse = callback;
      });

  auto result =
      ToPromise(MakeTestAnyExecutor(executor),
                static_cast<scada::CoroutineViewService&>(*audit).Browse(
                    {}, {scada::BrowseDescription{}}));
  Drain(executor);

  EXPECT_EQ(result.wait_for(0ms), promise_wait_status::timeout);
  ASSERT_TRUE(pending_browse);

  pending_browse(scada::StatusCode::Good, {});

  auto [status, browse_results] = WaitPromise(executor, std::move(result));
  EXPECT_TRUE(status.good());
  EXPECT_TRUE(browse_results.empty());
  EXPECT_TRUE(
      HasMetric(metric_service.Collect(executor), "browse_latency.count"));
}

TEST(AuditTest, AuditScadaServicesWithExecutorWrapsAttributeCallbacks) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  StrictMock<scada::MockAttributeService> attribute_service;
  auto source_services = std::make_shared<scada::services>(
      scada::services{.attribute_service = &attribute_service});
  auto audited_services =
      AuditScadaServices(source_services, metric_service, Tracer::None(),
                         MakeTestAnyExecutor(executor));

  ASSERT_NE(audited_services->attribute_service, nullptr);
  EXPECT_NE(audited_services->attribute_service, &attribute_service);

  scada::ReadCallback pending_read;
  EXPECT_CALL(attribute_service, Read(_, _, _))
      .WillOnce([&](const scada::ServiceContext&,
                    const std::shared_ptr<const std::vector<scada::ReadValueId>>&,
                    const scada::ReadCallback& callback) {
        pending_read = callback;
      });

  bool read_called = false;
  audited_services->attribute_service->Read(
      {}, std::make_shared<const std::vector<scada::ReadValueId>>(),
      [&](scada::Status status, std::vector<scada::DataValue>) {
        read_called = true;
        EXPECT_TRUE(status.good());
      });
  Drain(executor);

  EXPECT_FALSE(read_called);
  ASSERT_TRUE(pending_read);

  pending_read(scada::StatusCode::Good, {});
  Drain(executor);

  EXPECT_TRUE(read_called);
}

}  // namespace
