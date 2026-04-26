#pragma once

#include "base/any_executor.h"
#include "metrics/aggregated_metric.h"
#include "scada/attribute_service.h"
#include "scada/coroutine_services.h"
#include "scada/data_services.h"
#include "scada/services.h"
#include "scada/view_service.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

namespace scada {
struct services;
}

class MetricService;
class Tracer;

struct AuditContext {
  MetricService& metric_service_;
  DataServices data_services_;
  Tracer& tracer_;
  std::optional<AnyExecutor> executor_;
};

class Audit final : private AuditContext,
                    public scada::AttributeService,
                    public scada::ViewService,
                    public scada::CoroutineAttributeService,
                    public scada::CoroutineViewService,
                    public std::enable_shared_from_this<Audit> {
 public:
  static std::shared_ptr<Audit> Create(AuditContext&& context);

  // scada::AttributeService
  virtual void Read(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const scada::ServiceContext& context,
                      const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::CoroutineAttributeService
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

  // scada::CoroutineViewService
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

 private:
  explicit Audit(AuditContext&& context);

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  void Init();
  void RefreshCoroutineServices();
  void StartRead();
  void FinishRead(Clock::time_point start_time);
  void StartBrowse();
  void FinishBrowse(Clock::time_point start_time);

  mutable std::mutex mutex_;

  AggregatedMetric<Duration> read_latency_metric_;
  AggregatedMetric<Duration> browse_latency_metric_;

  AggregatedCounter<size_t> concurrent_read_count_;
  AggregatedCounter<size_t> concurrent_browse_count_;

  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      attribute_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineViewServiceAdapter>
      view_service_adapter_;

  scada::CoroutineAttributeService* coroutine_attribute_service_ = nullptr;
  scada::CoroutineViewService* coroutine_view_service_ = nullptr;
};

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer);

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer,
    AnyExecutor executor);

std::shared_ptr<DataServices> AuditDataServices(DataServices services,
                                                MetricService& metric_service,
                                                Tracer& tracer,
                                                AnyExecutor executor);
