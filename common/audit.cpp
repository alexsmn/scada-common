#include "common/audit.h"

#include "base/awaitable_promise.h"
#include "base/promise_executor.h"
#include "common/coroutine_service_resolver.h"
#include "common/data_services_util.h"
#include "metrics/metric_service.h"
#include "metrics/metrics.h"
#include "metrics/tracing.h"
#include "scada/services.h"
#include "scada/status_exception.h"
#include "scada/validation.h"

namespace {

std::shared_ptr<scada::services> AuditScadaServicesImpl(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer,
    std::optional<AnyExecutor> executor) {
  // |Audit| doesn't own underlying services.
  struct Holder {
    Holder(MetricService& metric_service,
           const std::shared_ptr<const scada::services>& services,
           Tracer& tracer,
           std::optional<AnyExecutor> executor)
        : services_{services},
          audit_{Audit::Create(AuditContext{
              .metric_service_ = metric_service,
              .data_services_ = data_services::FromUnownedServices(*services_),
              .tracer_ = tracer,
              .executor_ = std::move(executor)})},
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

  auto holder = std::make_shared<Holder>(metric_service, services, tracer,
                                         std::move(executor));
  return std::shared_ptr<scada::services>{holder, &holder->audited_services_};
}

std::shared_ptr<DataServices> AuditDataServicesImpl(
    DataServices services,
    MetricService& metric_service,
    Tracer& tracer,
    AnyExecutor executor) {
  struct Holder {
    Holder(MetricService& metric_service,
           DataServices services,
           Tracer& tracer,
           AnyExecutor executor)
        : services_{std::move(services)} {
      audit_ = Audit::Create(AuditContext{.metric_service_ = metric_service,
                                          .data_services_ = services_,
                                          .tracer_ = tracer,
                                          .executor_ = std::move(executor)});
      audited_services_ = services_;
      if (audited_services_.attribute_service_ ||
          audited_services_.coroutine_attribute_service_) {
        audited_services_.attribute_service_ = audit_;
        audited_services_.coroutine_attribute_service_ = audit_;
      }
      if (audited_services_.view_service_ ||
          audited_services_.coroutine_view_service_) {
        audited_services_.view_service_ = audit_;
        audited_services_.coroutine_view_service_ = audit_;
      }
    }

    DataServices services_;
    std::shared_ptr<Audit> audit_;
    DataServices audited_services_;
  };

  auto holder = std::make_shared<Holder>(metric_service, std::move(services),
                                         tracer, std::move(executor));
  return std::shared_ptr<DataServices>{holder, &holder->audited_services_};
}

}  // namespace

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer) {
  return AuditScadaServicesImpl(services, metric_service, tracer, std::nullopt);
}

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    MetricService& metric_service,
    Tracer& tracer,
    AnyExecutor executor) {
  return AuditScadaServicesImpl(services, metric_service, tracer,
                                std::move(executor));
}

std::shared_ptr<DataServices> AuditDataServices(DataServices services,
                                                MetricService& metric_service,
                                                Tracer& tracer,
                                                AnyExecutor executor) {
  return AuditDataServicesImpl(std::move(services), metric_service, tracer,
                               std::move(executor));
}

// Audit

Audit::Audit(AuditContext&& context)
    : AuditContext{std::move(context)},
      concurrent_read_count_{0},
      concurrent_browse_count_{0} {
  RefreshCoroutineServices();
}

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

void Audit::RefreshCoroutineServices() {
  coroutine_attribute_service_ =
      scada::service_resolver::ResolveCoroutineService(
          executor_, data_services_.coroutine_attribute_service_,
          data_services_.attribute_service_, attribute_service_adapter_);
  coroutine_view_service_ = scada::service_resolver::ResolveCoroutineService(
      executor_, data_services_.coroutine_view_service_,
      data_services_.view_service_, view_service_adapter_);
}

void Audit::StartRead() {
  std::lock_guard lock{mutex_};
  ++concurrent_read_count_;
}

void Audit::FinishRead(Clock::time_point start_time) {
  std::lock_guard lock{mutex_};
  --concurrent_read_count_;
  read_latency_metric_(Clock::now() - start_time);
}

void Audit::StartBrowse() {
  std::lock_guard lock{mutex_};
  ++concurrent_browse_count_;
}

void Audit::FinishBrowse(Clock::time_point start_time) {
  std::lock_guard lock{mutex_};
  --concurrent_browse_count_;
  browse_latency_metric_(Clock::now() - start_time);
}

void Audit::Read(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  if (executor_ && coroutine_attribute_service_) {
    CoSpawn(*executor_,
            [self = shared_from_this(), context, inputs,
             callback]() mutable -> Awaitable<void> {
              try {
                scada::CompleteStatusOrCallback(
                    callback,
                    co_await self->Read(std::move(context), std::move(inputs)));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  // Audit must not be used for missing services.
  assert(data_services_.attribute_service_);

  StartRead();

  data_services_.attribute_service_->Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        FinishRead(start_time);

        callback(std::move(status), std::move(results));
      });
}

void Audit::Write(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  if (executor_ && coroutine_attribute_service_) {
    CoSpawn(*executor_,
            [self = shared_from_this(), context, inputs,
             callback]() mutable -> Awaitable<void> {
              try {
                scada::CompleteStatusOrCallback(
                    callback,
                    co_await self->Write(std::move(context),
                                         std::move(inputs)));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  // Audit must not be used for missing services.
  assert(data_services_.attribute_service_);

  data_services_.attribute_service_->Write(context, inputs, callback);
}

void Audit::Browse(const scada::ServiceContext& context,
                   const std::vector<scada::BrowseDescription>& descriptions,
                   const scada::BrowseCallback& callback) {
  if (executor_ && coroutine_view_service_) {
    CoSpawn(*executor_,
            [self = shared_from_this(), context, descriptions,
             callback]() mutable -> Awaitable<void> {
              try {
                scada::CompleteStatusOrCallback(
                    callback,
                    co_await self->Browse(std::move(context), descriptions));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  // Audit must not be used for missing services.
  assert(data_services_.view_service_);

  StartBrowse();

  data_services_.view_service_->Browse(
      context, descriptions,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        assert(Validate(results));

        FinishBrowse(start_time);

        callback(std::move(status), std::move(results));
      });
}

void Audit::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  if (executor_ && coroutine_view_service_) {
    CoSpawn(*executor_,
            [self = shared_from_this(), browse_paths,
             callback]() mutable -> Awaitable<void> {
              try {
                scada::CompleteStatusOrCallback(
                    callback,
                    co_await self->TranslateBrowsePaths(browse_paths));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  // Audit must not be used for missing services.
  assert(data_services_.view_service_);

  data_services_.view_service_->TranslateBrowsePaths(browse_paths, callback);
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Audit::Read(
    scada::ServiceContext context,
    std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) {
  auto* service = coroutine_attribute_service_;
  if (service) {
    StartRead();
    const auto start_time = Clock::now();
    try {
      auto result =
          co_await service->Read(std::move(context), std::move(inputs));
      FinishRead(start_time);
      co_return result;
    } catch (...) {
      FinishRead(start_time);
      throw;
    }
  }

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::DataValue>{}};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
Audit::Write(scada::ServiceContext context,
             std::shared_ptr<const std::vector<scada::WriteValue>> inputs) {
  auto* service = coroutine_attribute_service_;
  if (service)
    co_return co_await service->Write(std::move(context), std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::StatusCode>{}};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
Audit::Browse(scada::ServiceContext context,
              std::vector<scada::BrowseDescription> inputs) {
  auto* service = coroutine_view_service_;
  if (service) {
    StartBrowse();
    const auto start_time = Clock::now();
    try {
      auto result =
          co_await service->Browse(std::move(context), std::move(inputs));
      assert(Validate(std::get<1>(result)));
      FinishBrowse(start_time);
      co_return result;
    } catch (...) {
      FinishBrowse(start_time);
      throw;
    }
  }

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::BrowseResult>{}};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
Audit::TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) {
  auto* service = coroutine_view_service_;
  if (service)
    co_return co_await service->TranslateBrowsePaths(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::BrowsePathResult>{}};
}
