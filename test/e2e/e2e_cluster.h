#pragma once

#include "test/e2e/e2e_server_process.h"

#include <boost/json/fwd.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace client::test {

// The tier-split server cluster (ADR 0001) shared by every E2E suite that needs
// a real multi-process server: a config tier owning the configuration
// namespace, three device edges (iec104 / modbus / iec61850, each a config
// client running one driver), a historian, a dedicated file store, and — on
// Windows — the Classic OPC and Vidicon module tiers.
//
// The client-facing aggregating proxy is deliberately NOT owned here. Each
// suite launches its own proxy process into its own slot (the client suite
// shares its workspace with the Qt client's scratch files), and shapes it with
// the ConfigureProxyRole() below so the routing-relevant configuration cannot
// drift between suites.
//
// Mirrors gcp/free-tier/multitier/configs/*.json — keep the two in step.

// Full-rights multi-session service account for inter-tier logins. The built-in
// root user is single-session (configuration_authenticator.cpp), so concurrent
// edge->config / edge->historian / proxy->edge logins would collide on
// Bad_UserIsAlreadyLoggedOn; `svc` (id 100, MultiSessions=1) does not. Mirrors
// gcp/free-tier/multitier/configs/seed-svc-user.sql.
extern const std::string_view kSvcUserSql;
extern const std::string_view kSvcUser;
extern const std::string_view kSvcPassword;

// Turns the analog item TIT.4 (which already references the RAMP simulation
// signal {9,3}) into a simulated, historized item collected into the analog
// historical DB {6,2}, so the server accumulates a steady stream of samples a
// client can read back.
extern const std::string_view kHistorizeSimulatedItemSql;

// The runtime node id of the historized item TIT.4 (analog item id 4 in the TIT
// namespace, index 2).
extern const std::string_view kHistorizedNodeId;

// Deletes a tier's data-item rows, so a tier that must not be authoritative for
// live values (the proxy, the file store) materialises none of them and the
// edges stay the single source.
extern const std::string_view kStripDataItemRowsSql;

// Appends {id:100, password:svc} to server.json's security.provision[] so a
// tier can authenticate inbound svc logins, preserving any existing entries
// (e.g. the fixture's guest id 12).
void ProvisionSvcPassword(boost::json::object& server_json);

// Applies the SCADA_SERVER_LICENSE_FILE (and optional
// SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING) environment license to a
// server.json object. The path is resolved to an absolute one against the TEST
// PROCESS's working directory: every tier is launched with its own temporary
// workspace as its working directory, so a relative value would not resolve
// there and the tier would start unlicensed.
void ConfigureSignedLicenseFromEnv(boost::json::object& server_json);

// Fails with an explanatory message when SCADA_SERVER_LICENSE_FILE is unset or
// points at a missing file. Call from a fixture's SetUp: without it a run fails
// much later, as an unexplained tier that never starts listening.
::testing::AssertionResult ValidateSignedLicenseEnv();

// Points one server process's metrics, traces and structured logs at
// `endpoint`, or leaves `server_json` untouched when it is empty. `service_name`
// becomes the process's OTel resource identity, so a cluster run shows up as one
// service per tier rather than several indistinguishable "scada-server" rows.
void ConfigureTelemetry(boost::json::object& server_json,
                        std::string_view service_name,
                        std::string_view endpoint);

// Which tier a cluster member is. The OTel service name is "scada-e2e-<name>"
// (ToString below), which is also how a trace assertion names the tier that
// answered a request.
enum class ClusterTier {
  kConfig,
  kHistorian,
  kIec104,
  kModbus,
  kIec61850,
  kFilesystem,
  kOpc,      // Windows only
  kVidicon,  // Windows only
};

// "config", "iec104", ... — the tier's binary short name without the "scada-"
// prefix, as used in docs/server/tier-namespace-map.md.
std::string_view ToString(ClusterTier tier);

// "scada-e2e-config", ... — the tier's OTel resource identity in an E2E run.
// Note the iec104 tier reports under its served group name ("iec60870"), not
// its binary name, matching the existing suites.
std::string TelemetryServiceName(ClusterTier tier);

// The tier binaries, injected by each E2E target from its own compile-time
// SCADA_E2E_*_EXE definitions. `opc` and `vidicon` are only consulted on
// Windows, and only when ClusterOptions::include_windows_module_tiers is set.
struct ClusterExecutables {
  std::filesystem::path config;
  std::filesystem::path historian;
  std::filesystem::path iec104;
  std::filesystem::path modbus;
  std::filesystem::path iec61850;
  std::filesystem::path filesystem;
  std::filesystem::path opc;
  std::filesystem::path vidicon;
};

struct ClusterOptions {
  // Port of the shared IEC 61850 test server the iec61850 edge polls; seeded
  // into the config tier's device config.
  int iec61850_port = 0;
  // The client-facing proxy's OPC UA URL. The dynamically-registered edge and
  // the historian call RegisterServer(2) back on it, so it must be known before
  // the tiers launch — allocate the proxy's ports first.
  std::string proxy_opcua_url;
  // Historize + simulate TIT.4 and give the historian a pull-collection source
  // on the iec104 edge.
  bool historize_simulated_item = false;
  // OTLP/gRPC endpoint every tier exports traces, metrics and logs to. Empty
  // (the CI default for the client suite) disables export entirely.
  std::string otlp_endpoint;
  // Launch the Windows-only Classic OPC and Vidicon module tiers as cluster
  // members and aggregate them behind the proxy. Ignored off Windows. Off by
  // default: the service x namespace sweep needs them to cover their matrix
  // rows, while the client suite's flows do not touch them and should not have
  // its Windows topology perturbed.
  bool include_windows_module_tiers = false;
};

// Owns the downstream tier processes of one cluster. Non-copyable and
// non-movable (each ServerTier owns a JobObject and a TempWorkspace); hold it
// by value in a fixture or via unique_ptr.
class ServerCluster {
 public:
  // `make_context` binds a tier binary to the calling target's fixture paths
  // and license policy (see ServerProcessContext).
  using MakeContextFn =
      std::function<ServerProcessContext(const std::filesystem::path& exe)>;

  ServerCluster(ClusterExecutables executables, MakeContextFn make_context);
  ~ServerCluster();

  ServerCluster(const ServerCluster&) = delete;
  ServerCluster& operator=(const ServerCluster&) = delete;

  // Launches config -> historian -> edges -> file store (and the Windows module
  // tiers when enabled), waiting for each to listen before the next depends on
  // it. Ports come from `ports`, so reserve the caller's proxy ports on it
  // first. Returns a failure describing the tier that did not come up, rather
  // than asserting, so it composes with any fixture.
  ::testing::AssertionResult Start(PortPool& ports,
                                   const ClusterOptions& options);

  // The launched tier, or nullptr when this cluster does not run it (the
  // Windows module tiers off Windows, or anything before Start()).
  ServerTier* Tier(ClusterTier tier) const;

  // Every launched tier, in reverse dependency order — edges and module tiers
  // before the config/historian they depend on. Safe to call before Start().
  std::vector<ServerTier*> TiersForShutdown() const;

  // The proxy's aggregation.servers[] entries for this cluster: the statically
  // aggregated edges, the claim-scoped file store, and the Windows module tiers
  // when enabled. The dynamically-registered edge and the historian are
  // deliberately absent — they arrive through RegisterServer(2) discovery.
  // Valid only after Start().
  boost::json::array AggregationServers() const;

  void PreserveWorkspaces();
  void Terminate();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

// How to shape a server.json object into the client-facing aggregating proxy.
struct ProxyRoleOptions {
  // Typically ServerCluster::AggregationServers().
  boost::json::array* aggregation_servers = nullptr;
  std::string otlp_endpoint;
  std::string_view service_name = "scada-e2e-proxy";
};

// Turns `server_json` into the client-facing aggregating proxy: no drivers, no
// local file store or history module of its own, data items off, the given
// aggregation downstreams, and a discovery-driven history link to whichever
// registrant advertises the "HD" capability.
//
// Shared by every suite that stands up a cluster, so the routing-relevant
// configuration — which downstream owns which namespace, and that history is
// the historian's — is defined in exactly one place. That configuration is what
// ADR 0003's namespace-claim work regressed twice; a second copy would be a
// second thing to regress.
void ConfigureProxyRole(boost::json::object& server_json,
                        const ProxyRoleOptions& options);

// Blocks until the proxy's log shows it has aggregated the
// RegisterServer-registered edge and linked the RegisterServer2 "HD" historian.
// Both registrants retry every 10 s and the proxy reconciles every 2 s, so a
// request issued right after startup would otherwise race the downstream coming
// up. Returns a failure naming the log directory on timeout.
::testing::AssertionResult WaitForProxyDownstreams(
    const std::filesystem::path& proxy_log_dir);

}  // namespace client::test
