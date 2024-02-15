#include "common/audit.h"

#include "base/promise_executor.h"
#include "metrics/metric_service.h"
#include "metrics/metrics.h"
#include "scada/validation.h"

Audit::Audit(AuditContext&& context) : AuditContext{std::move(context)} {}

void Audit::Init() {
  metric_service_.RegisterProvider(
      BindPromiseExecutor(executor_, weak_from_this(), [this] {
        Metrics metrics;
        metrics.Collect("read_latency", read_latency_metric_);
        metrics.Collect("browse_latency", browse_latency_metric_);
        return make_resolved_promise(metrics);
      }));
}

// static
std::shared_ptr<Audit> Audit::Create(AuditContext&& context) {
  auto audit = std::shared_ptr<Audit>(new Audit{std::move(context)});
  audit->Init();
  return audit;
}

void Audit::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  attribute_service_.Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        read_latency_metric_(Clock::now() - start_time);
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

        browse_latency_metric_(Clock::now() - start_time);

        callback(std::move(status), std::move(results));
      });
}

void Audit::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service_.TranslateBrowsePaths(browse_paths, callback);
}
