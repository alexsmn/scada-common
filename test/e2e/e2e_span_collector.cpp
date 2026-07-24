#include "test/e2e/e2e_span_collector.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include <opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.h>
#include <opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.h>
#include <opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h>
#include <opentelemetry/proto/common/v1/common.pb.h>

#include <iterator>
#include <mutex>
#include <thread>
#include <utility>

namespace client::test {

namespace {

namespace otlp_trace = ::opentelemetry::proto::collector::trace::v1;
namespace otlp_metrics = ::opentelemetry::proto::collector::metrics::v1;
namespace otlp_logs = ::opentelemetry::proto::collector::logs::v1;

// OTLP carries trace/span ids as raw bytes; every human-facing form (a W3C
// traceparent, a trace viewer's URL) is lower-case hex, so store them that way
// and the test can compare against the traceparent it minted.
std::string ToHex(const std::string& bytes) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(bytes.size() * 2);
  for (unsigned char byte : bytes) {
    hex.push_back(kDigits[byte >> 4]);
    hex.push_back(kDigits[byte & 0x0F]);
  }
  return hex;
}

// Renders an AnyValue as text. Span attributes the server sets are all strings
// (TraceSpan::SetAttribute takes one), but the resource carries mixed types and
// a future attribute should not silently read as empty.
std::string ToText(const ::opentelemetry::proto::common::v1::AnyValue& value) {
  using AnyValue = ::opentelemetry::proto::common::v1::AnyValue;
  switch (value.value_case()) {
    case AnyValue::kStringValue:
      return value.string_value();
    case AnyValue::kBoolValue:
      return value.bool_value() ? "true" : "false";
    case AnyValue::kIntValue:
      return std::to_string(value.int_value());
    case AnyValue::kDoubleValue:
      return std::to_string(value.double_value());
    default:
      return {};
  }
}

}  // namespace

std::string CollectedSpan::Attribute(std::string_view key) const {
  auto it = attributes.find(std::string{key});
  return it == attributes.end() ? std::string{} : it->second;
}

class SpanCollector::Impl {
 public:
  // Appends every span of an OTLP export, stamping each with the exporting
  // resource's service.name.
  void Ingest(const otlp_trace::ExportTraceServiceRequest& request) {
    std::vector<CollectedSpan> ingested;
    for (const auto& resource_spans : request.resource_spans()) {
      std::string service_name;
      for (const auto& attribute : resource_spans.resource().attributes()) {
        if (attribute.key() == "service.name") {
          service_name = ToText(attribute.value());
          break;
        }
      }
      for (const auto& scope_spans : resource_spans.scope_spans()) {
        for (const auto& span : scope_spans.spans()) {
          CollectedSpan collected{
              .service_name = service_name,
              .name = span.name(),
              .trace_id = ToHex(span.trace_id()),
              .span_id = ToHex(span.span_id()),
              .parent_span_id = ToHex(span.parent_span_id()),
          };
          for (const auto& attribute : span.attributes()) {
            collected.attributes.emplace(attribute.key(),
                                         ToText(attribute.value()));
          }
          ingested.push_back(std::move(collected));
        }
      }
    }

    if (ingested.empty())
      return;
    const std::lock_guard<std::mutex> lock{mutex_};
    spans_.insert(spans_.end(), std::make_move_iterator(ingested.begin()),
                  std::make_move_iterator(ingested.end()));
    last_arrival_ = std::chrono::steady_clock::now();
  }

  std::vector<CollectedSpan> Spans() const {
    const std::lock_guard<std::mutex> lock{mutex_};
    return spans_;
  }

  std::size_t Count() const {
    const std::lock_guard<std::mutex> lock{mutex_};
    return spans_.size();
  }

  std::chrono::steady_clock::time_point LastArrival() const {
    const std::lock_guard<std::mutex> lock{mutex_};
    return last_arrival_;
  }

  void Clear() {
    const std::lock_guard<std::mutex> lock{mutex_};
    spans_.clear();
    last_arrival_ = std::chrono::steady_clock::now();
  }

  // The trace service; the metric and log services below discard their payload
  // (one OTLP endpoint carries all three signals per tier, and an unimplemented
  // method would make the exporters retry noisily).
  class TraceService final : public otlp_trace::TraceService::Service {
   public:
    explicit TraceService(Impl& owner) : owner_{owner} {}

    ::grpc::Status Export(
        ::grpc::ServerContext*,
        const otlp_trace::ExportTraceServiceRequest* request,
        otlp_trace::ExportTraceServiceResponse*) override {
      if (request)
        owner_.Ingest(*request);
      return ::grpc::Status::OK;
    }

   private:
    Impl& owner_;
  };

  class MetricsService final : public otlp_metrics::MetricsService::Service {
   public:
    ::grpc::Status Export(
        ::grpc::ServerContext*,
        const otlp_metrics::ExportMetricsServiceRequest*,
        otlp_metrics::ExportMetricsServiceResponse*) override {
      return ::grpc::Status::OK;
    }
  };

  class LogsService final : public otlp_logs::LogsService::Service {
   public:
    ::grpc::Status Export(::grpc::ServerContext*,
                          const otlp_logs::ExportLogsServiceRequest*,
                          otlp_logs::ExportLogsServiceResponse*) override {
      return ::grpc::Status::OK;
    }
  };

  // Binds an ephemeral loopback port and starts serving. Returns the endpoint,
  // or an empty string when the bind failed.
  std::string Start() {
    int port = 0;
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", ::grpc::InsecureServerCredentials(),
                             &port);
    builder.RegisterService(&trace_service_);
    builder.RegisterService(&metrics_service_);
    builder.RegisterService(&logs_service_);
    server_ = builder.BuildAndStart();
    if (!server_ || port == 0)
      return {};
    return "127.0.0.1:" + std::to_string(port);
  }

  void Shutdown() {
    if (server_)
      server_->Shutdown();
  }

 private:
  mutable std::mutex mutex_;
  std::vector<CollectedSpan> spans_;
  std::chrono::steady_clock::time_point last_arrival_ =
      std::chrono::steady_clock::now();

  TraceService trace_service_{*this};
  MetricsService metrics_service_;
  LogsService logs_service_;
  std::unique_ptr<::grpc::Server> server_;
};

SpanCollector::SpanCollector() : impl_{std::make_unique<Impl>()} {
  endpoint_ = impl_->Start();
}

SpanCollector::~SpanCollector() {
  impl_->Shutdown();
}

std::vector<CollectedSpan> SpanCollector::Spans() const {
  return impl_->Spans();
}

std::vector<CollectedSpan> SpanCollector::SpansOnTrace(
    std::string_view trace_id,
    std::string_view span_name) const {
  std::vector<CollectedSpan> matches;
  for (CollectedSpan& span : impl_->Spans()) {
    if (span.trace_id != trace_id)
      continue;
    if (!span_name.empty() && span.name != span_name)
      continue;
    matches.push_back(std::move(span));
  }
  return matches;
}

std::set<std::string> SpanCollector::ServicesEmitting(
    std::string_view trace_id,
    std::string_view span_name) const {
  std::set<std::string> services;
  for (const CollectedSpan& span : SpansOnTrace(trace_id, span_name))
    services.insert(span.service_name);
  return services;
}

std::vector<CollectedSpan> SpanCollector::SpansByAttribute(
    std::string_view span_name,
    std::string_view attribute_key,
    std::string_view attribute_value) const {
  std::vector<CollectedSpan> matches;
  for (CollectedSpan& span : impl_->Spans()) {
    if (span.name != span_name)
      continue;
    if (span.Attribute(attribute_key) != attribute_value)
      continue;
    matches.push_back(std::move(span));
  }
  return matches;
}

std::size_t SpanCollector::Count() const {
  return impl_->Count();
}

bool SpanCollector::WaitFor(
    const std::function<bool(const SpanCollector&)>& predicate,
    std::chrono::milliseconds timeout) const {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate(*this))
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  return predicate(*this);
}

bool SpanCollector::WaitUntilQuiet(std::chrono::milliseconds quiet_period,
                                   std::chrono::milliseconds timeout) const {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::chrono::steady_clock::now() - impl_->LastArrival() >= quiet_period)
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  return std::chrono::steady_clock::now() - impl_->LastArrival() >= quiet_period;
}

void SpanCollector::Clear() {
  impl_->Clear();
}

}  // namespace client::test
