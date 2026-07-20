#include "common/audit.h"

#include "common/data_services_util.h"
#include "metrics/tracing.h"
#include "scada/services.h"

namespace {

std::shared_ptr<scada::services> AuditScadaServicesImpl(
    const std::shared_ptr<const scada::services>& services,
    Tracer& tracer,
    std::optional<AnyExecutor> executor) {
  // |Audit| doesn't own underlying services.
  struct Holder {
    Holder(const std::shared_ptr<const scada::services>& services,
           Tracer& tracer,
           std::optional<AnyExecutor> executor)
        : services_{services},
          audit_{Audit::Create(AuditContext{
              .data_services_ =
                  scada::data_services::FromUnownedServices(*services_),
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

  auto holder =
      std::make_shared<Holder>(services, tracer, std::move(executor));
  return std::shared_ptr<scada::services>{holder, &holder->audited_services_};
}

std::shared_ptr<DataServices> AuditDataServicesImpl(
    DataServices services,
    Tracer& tracer,
    AnyExecutor executor) {
  struct Holder {
    Holder(DataServices services,
           Tracer& tracer,
           AnyExecutor executor)
        : services_{std::move(services)} {
      audit_ = Audit::Create(AuditContext{.data_services_ = services_,
                                          .tracer_ = tracer,
                                          .executor_ = std::move(executor)});
      audited_services_ = services_;
      if (audited_services_.attribute_service_) {
        audited_services_.attribute_service_ = audit_;
      }
      if (audited_services_.view_service_) {
        audited_services_.view_service_ = audit_;
      }
    }

    DataServices services_;
    std::shared_ptr<Audit> audit_;
    DataServices audited_services_;
  };

  auto holder =
      std::make_shared<Holder>(std::move(services), tracer, std::move(executor));
  return std::shared_ptr<DataServices>{holder, &holder->audited_services_};
}

}  // namespace

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    Tracer& tracer) {
  return AuditScadaServicesImpl(services, tracer, std::nullopt);
}

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    Tracer& tracer,
    AnyExecutor executor) {
  return AuditScadaServicesImpl(services, tracer, std::move(executor));
}

std::shared_ptr<DataServices> AuditDataServices(DataServices services,
                                                Tracer& tracer,
                                                AnyExecutor executor) {
  return AuditDataServicesImpl(std::move(services), tracer, std::move(executor));
}

// Audit

Audit::Audit(AuditContext&& context)
    : AuditContext{std::move(context)} {
  RefreshCoroutineServices();
}

// static
std::shared_ptr<Audit> Audit::Create(AuditContext&& context) {
  return std::shared_ptr<Audit>(new Audit{std::move(context)});
}

void Audit::RefreshCoroutineServices() {
  attribute_service_ = data_services_.attribute_service_.get();
  view_service_ = data_services_.view_service_.get();
}

void Audit::StartRead() {
  metrics_.AddUpDownCounter("scada.audit.concurrent_read_count", 1);
}

void Audit::FinishRead(Clock::time_point start_time) {
  metrics_.AddUpDownCounter("scada.audit.concurrent_read_count", -1);
  metrics_.RecordHistogram(
      "scada.audit.read_latency_ms",
      std::chrono::duration<double, std::milli>{Clock::now() - start_time}
          .count());
}

void Audit::StartBrowse() {
  metrics_.AddUpDownCounter("scada.audit.concurrent_browse_count", 1);
}

void Audit::FinishBrowse(Clock::time_point start_time) {
  metrics_.AddUpDownCounter("scada.audit.concurrent_browse_count", -1);
  metrics_.RecordHistogram(
      "scada.audit.browse_latency_ms",
      std::chrono::duration<double, std::milli>{Clock::now() - start_time}
          .count());
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Audit::Read(
    scada::ServiceContext context,
    std::vector<scada::ReadValueId> inputs) {
  auto* service = attribute_service_;
  if (service) {
    StartRead();
    const auto start_time = Clock::now();
    auto result = co_await service->Read(std::move(context), std::move(inputs));
    FinishRead(start_time);
    co_return result;
  }

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
Audit::Write(scada::ServiceContext context,
             std::vector<scada::WriteValue> inputs) {
  auto* service = attribute_service_;
  if (service)
    co_return co_await service->Write(std::move(context), std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
Audit::Browse(scada::ServiceContext context,
              std::vector<scada::BrowseDescription> inputs) {
  auto* service = view_service_;
  if (service) {
    StartBrowse();
    const auto start_time = Clock::now();
    auto result =
        co_await service->Browse(std::move(context), std::move(inputs));
    // No validity enforcement here: the wrapped service may be a remote
    // proxy, and malformed remote data is handled by the consumers.
    FinishBrowse(start_time);
    co_return result;
  }

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
Audit::TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) {
  auto* service = view_service_;
  if (service)
    co_return co_await service->TranslateBrowsePaths(std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}
