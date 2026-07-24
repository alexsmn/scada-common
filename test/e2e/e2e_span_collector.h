#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace client::test {

// One exported OpenTelemetry span, flattened out of the OTLP wire format.
//
// `service_name` is the exporting process's `service.name` resource attribute —
// in an E2E run, "scada-e2e-config", "scada-e2e-proxy", ... — which is what
// makes a span *evidence about which tier answered a request*. That is the
// whole point of collecting these: for a routed request, "returned Good" says
// nothing about where it was served, but a span on scada-e2e-historian is proof
// the historian handled it.
struct CollectedSpan {
  std::string service_name;
  // "opcua.server/Read", "scada.route/Write", ... — see
  // scada-server-framework/docs/tracing.md for the inventory.
  std::string name;
  // Lower-case hex, as they appear in a W3C traceparent.
  std::string trace_id;
  std::string span_id;
  std::string parent_span_id;
  // String-valued attributes only (`scada.node_ids`, `scada.route.count`, ...).
  // Non-string values are rendered with their natural textual form.
  std::map<std::string, std::string> attributes;

  // The attribute's value, or an empty string when absent.
  std::string Attribute(std::string_view key) const;
};

// An in-process OTLP/gRPC receiver: everything the tiers of a cluster export
// lands in memory in the test process, so a trace assertion needs no external
// collector, no docker, and no dev/telemetry bridge.
//
// It implements the trace service plus no-op metric and log services, because
// one OTLP endpoint carries all three signals for a tier (see
// scada-server-framework/docs/tracing.md, "Configuration") and an unimplemented
// method would make every exporter retry noisily.
//
// Thread-safe: gRPC delivers Export calls on its own threads while the test
// thread queries.
//
// Note on latency: the tiers batch spans through a default BatchSpanProcessor
// (~5 s schedule delay, core/metrics/otel_traces.cpp), so spans arrive well
// after the request that caused them returned. Issue all the probes of a pass
// first and settle once with WaitFor — never once per probe.
class SpanCollector {
 public:
  SpanCollector();
  ~SpanCollector();

  SpanCollector(const SpanCollector&) = delete;
  SpanCollector& operator=(const SpanCollector&) = delete;

  // "127.0.0.1:<port>" — feed this to ClusterOptions::otlp_endpoint. Empty when
  // the receiver failed to bind (Started() is then false).
  const std::string& endpoint() const { return endpoint_; }
  bool Started() const { return !endpoint_.empty(); }

  // Every span received so far.
  std::vector<CollectedSpan> Spans() const;

  // Spans matching a trace id and (when non-empty) a span name.
  std::vector<CollectedSpan> SpansOnTrace(std::string_view trace_id,
                                          std::string_view span_name = {}) const;

  // The distinct service names that emitted `span_name` on `trace_id` — i.e.
  // the set of tiers that handled that request. This is the assertion the
  // service x namespace sweep is built on.
  std::set<std::string> ServicesEmitting(std::string_view trace_id,
                                         std::string_view span_name) const;

  // Spans named `span_name`, from `service_name`, carrying `attribute_value`
  // under `attribute_key`. The fallback matcher for the two services whose
  // spans cannot be trace-correlated: HistoryReadRaw / HistoryReadEvents carry
  // no ServiceContext, so a proxy->historian history read starts a new trace
  // root (scada-server-framework/docs/tracing.md, "Known gaps"). A probe with a
  // unique node id is still attributable through `scada.node_id`.
  std::vector<CollectedSpan> SpansByAttribute(
      std::string_view span_name,
      std::string_view attribute_key,
      std::string_view attribute_value) const;

  // Total spans received; useful to detect "nothing was exported at all"
  // (a misconfigured endpoint) as distinct from "the request went elsewhere".
  std::size_t Count() const;

  // Blocks until `predicate` holds over the collected spans or `timeout`
  // elapses. Returns the predicate's final value.
  bool WaitFor(const std::function<bool(const SpanCollector&)>& predicate,
               std::chrono::milliseconds timeout) const;

  // Blocks until no new span has arrived for `quiet_period`, or `timeout`
  // elapses. Use after firing a batch of probes: the tiers' batch processors
  // flush on their own schedule, so "nothing new for a while" is a better
  // settle signal than a fixed sleep. Returns false if it gave up on `timeout`
  // while spans were still arriving.
  bool WaitUntilQuiet(std::chrono::milliseconds quiet_period,
                      std::chrono::milliseconds timeout) const;

  void Clear();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
  std::string endpoint_;
};

}  // namespace client::test
