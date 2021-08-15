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
        std::shared_ptr<scada::ViewService> view_service);

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

  boost::asio::io_context& io_context_;
  const std::shared_ptr<scada::AttributeService> attribute_service_;
  const std::shared_ptr<scada::ViewService> view_service_;

  boost::asio::io_context::strand executor_{io_context_};

  Metric<Duration> read_metric_;
  Metric<Duration> browse_metric_;

  const std::shared_ptr<AuditLogger> logger_;
  Timer timer_{io_context_};
};
