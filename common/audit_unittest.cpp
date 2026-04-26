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

class TestCoroutineAuditServices final
    : public scada::CoroutineAttributeService,
      public scada::CoroutineViewService {
 public:
  Awaitable<std::tuple<scada::Status, std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override {
    ++read_count;
    last_read_inputs = *inputs;
    co_return std::tuple{scada::Status{scada::StatusCode::Good}, read_results};
  }

  Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override {
    ++write_count;
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::vector<scada::StatusCode>(
                             inputs->size(), scada::StatusCode::Good)};
  }

  Awaitable<std::tuple<scada::Status, std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> inputs) override {
    ++browse_count;
    last_browse_inputs = std::move(inputs);
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         browse_results};
  }

  Awaitable<std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override {
    ++translate_count;
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::vector<scada::BrowsePathResult>(inputs.size())};
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

DataServices MakeAuditDataServices(const scada::services& services) {
  return DataServices::FromUnownedServices(services);
}

TEST(AuditTest, CallbackReadUsesCoroutineAdapterAndRecordsMetric) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();
  scada::services services{.attribute_service = attribute_service.get()};
  auto audit = Audit::Create(
      AuditContext{.metric_service_ = metric_service,
                   .data_services_ = MakeAuditDataServices(services),
                   .tracer_ = Tracer::None(),
                   .executor_ = MakeTestAnyExecutor(executor)});

  scada::ReadCallback pending_read;
  EXPECT_CALL(*attribute_service, Read(_, _, _))
      .WillOnce(
          [&](const scada::ServiceContext&,
              const std::shared_ptr<const std::vector<scada::ReadValueId>>&,
              const scada::ReadCallback& callback) {
            pending_read = callback;
          });

  bool read_called = false;
  audit->Read({}, std::make_shared<const std::vector<scada::ReadValueId>>(),
              [&](scada::Status status, std::vector<scada::DataValue> results) {
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
  EXPECT_TRUE(
      HasMetric(metric_service.Collect(executor), "read_latency.count"));
}

TEST(AuditTest, CoroutineBrowseUsesCallbackAdapterAndRecordsMetric) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  auto view_service = std::make_shared<StrictMock<scada::MockViewService>>();
  scada::services services{.view_service = view_service.get()};
  auto audit = Audit::Create(
      AuditContext{.metric_service_ = metric_service,
                   .data_services_ = MakeAuditDataServices(services),
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
      .WillOnce(
          [&](const scada::ServiceContext&,
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

TEST(AuditTest, AuditDataServicesWrapsDirectCoroutineSlots) {
  auto executor = std::make_shared<TestExecutor>();
  CapturingMetricService metric_service;
  auto source_services = std::make_shared<TestCoroutineAuditServices>();

  DataServices data_services;
  data_services.coroutine_attribute_service_ = source_services;
  data_services.coroutine_view_service_ = source_services;

  auto audited_services =
      AuditDataServices(std::move(data_services), metric_service,
                        Tracer::None(), MakeTestAnyExecutor(executor));

  ASSERT_NE(audited_services->attribute_service_, nullptr);
  ASSERT_NE(audited_services->coroutine_attribute_service_, nullptr);
  ASSERT_NE(audited_services->view_service_, nullptr);
  ASSERT_NE(audited_services->coroutine_view_service_, nullptr);
  EXPECT_NE(audited_services->coroutine_attribute_service_.get(),
            source_services.get());
  EXPECT_NE(audited_services->coroutine_view_service_.get(),
            source_services.get());

  bool read_called = false;
  auto read_inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{{.node_id = scada::NodeId{1}}});
  audited_services->attribute_service_->Read(
      {}, read_inputs,
      [&](scada::Status status, std::vector<scada::DataValue> results) {
        read_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results, source_services->read_results);
      });
  Drain(executor);

  EXPECT_TRUE(read_called);
  EXPECT_EQ(source_services->read_count, 1);
  EXPECT_EQ(source_services->last_read_inputs, *read_inputs);
  EXPECT_TRUE(
      HasMetric(metric_service.Collect(executor), "read_latency.count"));

  bool write_called = false;
  auto write_inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{{.node_id = scada::NodeId{4}}});
  audited_services->attribute_service_->Write(
      {}, write_inputs,
      [&](scada::Status status, std::vector<scada::StatusCode> results) {
        write_called = true;
        EXPECT_TRUE(status.good());
        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0], scada::StatusCode::Good);
      });
  Drain(executor);

  EXPECT_TRUE(write_called);
  EXPECT_EQ(source_services->write_count, 1);

  auto [browse_status, browse_results] =
      WaitAwaitable(executor, audited_services->coroutine_view_service_->Browse(
                                  {}, {{.node_id = scada::NodeId{2}}}));

  EXPECT_TRUE(browse_status.good());
  EXPECT_EQ(browse_results, source_services->browse_results);
  EXPECT_EQ(source_services->browse_count, 1);
  ASSERT_EQ(source_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(source_services->last_browse_inputs[0].node_id, (scada::NodeId{2}));
  EXPECT_TRUE(
      HasMetric(metric_service.Collect(executor), "browse_latency.count"));

  bool browse_called = false;
  audited_services->view_service_->Browse(
      {}, {{.node_id = scada::NodeId{3}}},
      [&](scada::Status status, std::vector<scada::BrowseResult> results) {
        browse_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results, source_services->browse_results);
      });
  Drain(executor);

  EXPECT_TRUE(browse_called);
  EXPECT_EQ(source_services->browse_count, 2);
  ASSERT_EQ(source_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(source_services->last_browse_inputs[0].node_id, (scada::NodeId{3}));
  EXPECT_TRUE(
      HasMetric(metric_service.Collect(executor), "browse_latency.count"));

  bool translate_called = false;
  audited_services->view_service_->TranslateBrowsePaths(
      {scada::BrowsePath{}},
      [&](scada::Status status,
          std::vector<scada::BrowsePathResult> results) {
        translate_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(translate_called);
  EXPECT_EQ(source_services->translate_count, 1);
}

}  // namespace
