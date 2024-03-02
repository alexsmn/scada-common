#include "common/audit.h"

#include "base/promise_executor.h"
#include "metrics/metric_service.h"
#include "metrics/metrics.h"
#include "metrics/tracing.h"
#include "scada/validation.h"

Audit::Audit(AuditContext&& context) : AuditContext{std::move(context)} {}

void Audit::Init() {
  metric_service_.RegisterProvider(
      BindPromiseCancelation(weak_from_this(), [this] {
        std::lock_guard lock{mutex_};
        Metrics metrics;
        metrics.Set("read_latency", read_latency_metric_);
        metrics.Set("browse_latency", browse_latency_metric_);
        metrics.Set("concurrent_read_count", concurrent_read_count_.metric);
        metrics.Set("concurrent_browse_count", concurrent_browse_count_.metric);
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
  {
    std::lock_guard lock{mutex_};
    ++concurrent_read_count_;
  }

  attribute_service_.Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback,
       trace_span = std::make_shared<TraceSpan>(root_trace_span_.StartSpan())](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        {
          std::lock_guard lock{mutex_};
          --concurrent_read_count_;
          read_latency_metric_(Clock::now() - start_time);
        }

        callback(std::move(status), std::move(results));
      });
}

void Audit::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  attribute_service_.Write(context, inputs, callback);
}

void Audit::Browse(const std::shared_ptr<const scada::ServiceContext>& context,
                   const std::vector<scada::BrowseDescription>& descriptions,
                   const scada::BrowseCallback& callback) {
  {
    std::lock_guard lock{mutex_};
    ++concurrent_browse_count_;
  }

  view_service_.Browse(
      context, descriptions,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        assert(Validate(results));

        {
          std::lock_guard lock{mutex_};
          --concurrent_browse_count_;
          browse_latency_metric_(Clock::now() - start_time);
        }

        callback(std::move(status), std::move(results));
      });
}

void Audit::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service_.TranslateBrowsePaths(browse_paths, callback);
}
