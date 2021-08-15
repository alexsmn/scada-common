#include "common/audit.h"

#include "base/containers/span.h"

namespace {

bool Validate(base::span<const scada::ReferenceDescription> references) {
  std::vector<scada::ReferenceDescription> sorted_references(references.begin(),
                                                             references.end());
  std::sort(sorted_references.begin(), sorted_references.end(),
            [](const scada::ReferenceDescription& a,
               const scada::ReferenceDescription& b) {
              return std::tie(a.reference_type_id, a.forward, a.node_id) <
                     std::tie(b.reference_type_id, b.forward, b.node_id);
            });
  bool contain_duplicates =
      std::adjacent_find(sorted_references.begin(), sorted_references.end(),
                         std::equal_to{}) != sorted_references.end();
  assert(!contain_duplicates);
  return !contain_duplicates;
}

bool Validate(const scada::BrowseResult& result) {
  if (scada::IsGood(result.status_code)) {
    return Validate(result.references);
  } else {
    assert(result.references.empty());
    return result.references.empty();
  }
}

bool Validate(base::span<const scada::BrowseResult> results) {
  return std::all_of(
      results.begin(), results.end(),
      [](const scada::BrowseResult& result) { return Validate(result); });
}

}  // namespace

Audit::Audit(boost::asio::io_context& io_context,
             std::shared_ptr<AuditLogger> logger,
             std::shared_ptr<scada::AttributeService> attribute_service,
             std::shared_ptr<scada::ViewService> view_service)
    : io_context_{io_context},
      logger_{std::move(logger)},
      attribute_service_{std::move(attribute_service)},
      view_service_{std::move(view_service)} {
  timer_.StartRepeating(std::chrono::minutes{1}, executor_.wrap([this] {
    LogAndReset("Browse.DurationUs", browse_metric_);
    LogAndReset("Read.DurationUs", read_metric_);
  }));
}

void Audit::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  attribute_service_->Read(
      context, inputs,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        executor_.dispatch([this, ref, duration = Clock::now() - start_time] {
          read_metric_(duration);
        });
        callback(std::move(status), std::move(results));
      });
}

void Audit::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  attribute_service_->Write(context, inputs, callback);
}

void Audit::Browse(const std::vector<scada::BrowseDescription>& descriptions,
                   const scada::BrowseCallback& callback) {
  view_service_->Browse(
      descriptions,
      [this, ref = shared_from_this(), start_time = Clock::now(), callback](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        assert(Validate(results));

        executor_.dispatch([this, ref, duration = Clock::now() - start_time] {
          browse_metric_(duration);
        });

        callback(std::move(status), std::move(results));
      });
}

void Audit::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service_->TranslateBrowsePaths(browse_paths, callback);
}

void Audit::LogAndReset(const char* name, Metric<Duration>& metric) {
  if (metric.empty())
    return;

  logger_->Log(name, metric);

  metric.reset();
}
