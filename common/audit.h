#pragma once

#include "base/logger.h"
#include "base/metric.h"
#include "base/timer.h"
#include "common/audit_logger.h"
#include "core/attribute_service.h"
#include "core/view_service.h"

#include <boost/asio/io_context_strand.hpp>
#include <memory>

class Audit final : public scada::AttributeService,
                    public scada::ViewService,
                    public std::enable_shared_from_this<Audit> {
 public:
  Audit(boost::asio::io_context& io_context,
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

  // scada::AttributeService

  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override {
    attribute_service_->Read(
        value_ids,
        [this, ref = shared_from_this(), start_time = Clock::now(), callback](
            scada::Status&& status, std::vector<scada::DataValue>&& results) {
          executor_.dispatch([this, ref, duration = Clock::now() - start_time] {
            read_metric_(duration);
          });
          callback(std::move(status), std::move(results));
        });
  }

  virtual void Write(const scada::WriteValue& value,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) override {
    attribute_service_->Write(value, user_id, callback);
  }

  // scada::ViewService

  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override {
    view_service_->Browse(
        descriptions,
        [this, ref = shared_from_this(), start_time = Clock::now(), callback](
            scada::Status&& status,
            std::vector<scada::BrowseResult>&& results) {
          executor_.dispatch([this, ref, duration = Clock::now() - start_time] {
            browse_metric_(duration);
          });
          callback(std::move(status), std::move(results));
        });
  }

  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override {
    view_service_->TranslateBrowsePath(starting_node_id, relative_path,
                                       callback);
  }

  virtual void Subscribe(scada::ViewEvents& events) override {
    view_service_->Subscribe(events);
  }

  virtual void Unsubscribe(scada::ViewEvents& events) override {
    view_service_->Unsubscribe(events);
  }

 private:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  void LogAndReset(const char* name, Metric<Duration>& metric) {
    if (metric.empty())
      return;

    logger_->Log(name, metric);

    metric.reset();
  }

  boost::asio::io_context& io_context_;
  const std::shared_ptr<scada::AttributeService> attribute_service_;
  const std::shared_ptr<scada::ViewService> view_service_;

  boost::asio::io_context::strand executor_{io_context_};

  Metric<Duration> read_metric_;
  Metric<Duration> browse_metric_;

  const std::shared_ptr<AuditLogger> logger_;
  Timer timer_{io_context_};
};
