#pragma once

#include "base/any_executor.h"
#include "metrics/otel_metrics.h"
#include "scada/attribute_service.h"
#include "scada/data_services.h"
#include "scada/services.h"
#include "scada/view_service.h"

#include <chrono>
#include <memory>
#include <optional>

namespace scada {
struct services;
}

class Tracer;

struct AuditContext {
  DataServices data_services_;
  Tracer& tracer_;
  std::optional<AnyExecutor> executor_;
};

class Audit final : private AuditContext,
                    public scada::AttributeService,
                    public scada::ViewService,
                    public std::enable_shared_from_this<Audit> {
 public:
  static std::shared_ptr<Audit> Create(AuditContext&& context);

  // scada::AttributeService
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::vector<scada::ReadValueId> inputs) override;
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::vector<scada::WriteValue> inputs) override;

  // scada::ViewService
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  [[nodiscard]] virtual Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

 private:
  explicit Audit(AuditContext&& context);

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  void RefreshCoroutineServices();
  void StartRead();
  void FinishRead(Clock::time_point start_time);
  void StartBrowse();
  void FinishBrowse(Clock::time_point start_time);

  scada::metrics::Meter metrics_{"scada.audit"};

  scada::AttributeService* attribute_service_ = nullptr;
  scada::ViewService* view_service_ = nullptr;
};

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    Tracer& tracer);

std::shared_ptr<scada::services> AuditScadaServices(
    const std::shared_ptr<const scada::services>& services,
    Tracer& tracer,
    AnyExecutor executor);

std::shared_ptr<DataServices> AuditDataServices(DataServices services,
                                                Tracer& tracer,
                                                AnyExecutor executor);
