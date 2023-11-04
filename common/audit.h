#pragma once

#include "metrics/aggregated_metric.h"
#include "scada/attribute_service.h"
#include "scada/view_service.h"

#include <chrono>
#include <memory>

class Executor;
class MetricService;

struct AuditContext {
  std::shared_ptr<Executor> executor_;
  MetricService& metric_service_;
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
};

class Audit final : private AuditContext,
                    public scada::AttributeService,
                    public scada::ViewService,
                    public std::enable_shared_from_this<Audit> {
 public:
  static std::shared_ptr<Audit> Create(AuditContext&& context);

  // scada::AttributeService
  virtual void Read(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

 private:
  explicit Audit(AuditContext&& context);

  void Init();

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  // The metrics must be accessed under the `executor_`.
  AggregatedMetric<Duration> read_latency_metric_;
  AggregatedMetric<Duration> browse_latency_metric_;
};
