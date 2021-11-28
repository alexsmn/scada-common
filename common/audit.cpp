#include "common/audit.h"

#include "core/validation.h"

Audit::Audit(std::shared_ptr<Executor> executor,
             std::shared_ptr<AuditLogger> logger,
             scada::AttributeService& attribute_service,
             scada::ViewService& view_service)
    : executor_{std::move(executor)},
      logger_{std::move(logger)},
      attribute_service_{attribute_service},
      view_service_{view_service} {
  // | timer_| is bound to |executor_| and limited by the object scope.
  timer_.StartRepeating(std::chrono::minutes{1}, [this] {
    LogAndReset("Browse.DurationUs", browse_metric_);
    LogAndReset("Read.DurationUs", read_metric_);
  });
}

void Audit::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  attribute_service_.Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        Dispatch(*executor_, [this, ref, duration = Clock::now() - start_time] {
          read_metric_(duration);
        });
        callback(std::move(status), std::move(results));
      });
}

void Audit::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  attribute_service_.Write(context, inputs, callback);
}

void Audit::Browse(const std::vector<scada::BrowseDescription>& descriptions,
                   const scada::BrowseCallback& callback) {
  view_service_.Browse(
      descriptions,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        assert(Validate(results));

        Dispatch(*executor_, [this, ref, duration = Clock::now() - start_time] {
          browse_metric_(duration);
        });

        callback(std::move(status), std::move(results));
      });
}

void Audit::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service_.TranslateBrowsePaths(browse_paths, callback);
}

void Audit::LogAndReset(const char* name, Metric<Duration>& metric) {
  if (metric.empty())
    return;

  logger_->Log(name, metric);

  metric.reset();
}
