#pragma once

#include "base/executor.h"
#include "base/executor_timer.h"
#include "base/metric.h"
#include "common/audit_logger.h"
#include "scada/attribute_service.h"
#include "scada/view_service.h"

#include <memory>

class Audit final : public scada::AttributeService,
                    public scada::ViewService,
                    public std::enable_shared_from_this<Audit> {
 public:
  Audit(std::shared_ptr<Executor> executor,
        std::shared_ptr<AuditLogger> logger,
        scada::AttributeService& attribute_service,
        scada::ViewService& view_service);

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
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  void LogAndReset(const char* name, Metric<Duration>& metric);

  const std::shared_ptr<Executor> executor_;
  const std::shared_ptr<AuditLogger> logger_;
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;

  Metric<Duration> read_metric_;
  Metric<Duration> browse_metric_;

  ExecutorTimer timer_{executor_};
};
