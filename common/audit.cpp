#include "common/audit.h"

#include "base/promise_executor.h"
#include "metrics/metric_service.h"
#include "metrics/metrics.h"
#include "metrics/tracing.h"
#include "scada/services.h"
#include "scada/validation.h"

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer) {
  // |Audit| doesn't own underlying services.
  struct Holder {
    Holder(MetricService& metric_service,
           const std::shared_ptr<const scada::services>& services,
           Tracer& tracer)
        : services_{services},
          audit_{
              Audit::Create(AuditContext{metric_service, *services_, tracer})},
          audited_services_{*services_} {
      if (audited_services_.attribute_service) {
        audited_services_.attribute_service = audit_.get();
      }

      if (audited_services_.view_service) {
        audited_services_.view_service = audit_.get();
      }
    }

    std::shared_ptr<const scada::services> services_;
    std::shared_ptr<Audit> audit_;
    scada::services audited_services_;
  };

  auto holder = std::make_shared<Holder>(metric_service, services, tracer);
  return std::shared_ptr<scada::services>{holder, &holder->audited_services_};
}

// Audit

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
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  // Audit must not be used for missing services.
  assert(services_.attribute_service);

  {
    std::lock_guard lock{mutex_};
    ++concurrent_read_count_;
  }

  services_.attribute_service->Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
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
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  // Audit must not be used for missing services.
  assert(services_.attribute_service);

  services_.attribute_service->Write(context, inputs, callback);
}

void Audit::Browse(const scada::ServiceContext& context,
                   const std::vector<scada::BrowseDescription>& descriptions,
                   const scada::BrowseCallback& callback) {
  // Audit must not be used for missing services.
  assert(services_.view_service);

  {
    std::lock_guard lock{mutex_};
    ++concurrent_browse_count_;
  }

  services_.view_service->Browse(
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
  // Audit must not be used for missing services.
  assert(services_.view_service);

  services_.view_service->TranslateBrowsePaths(browse_paths, callback);
}
