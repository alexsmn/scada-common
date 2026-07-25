#include "test/e2e/e2e_cluster.h"

#include "test/e2e/e2e_file_helpers.h"

#include <boost/json.hpp>

#include <chrono>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace client::test {

const std::string_view kSvcUserSql =
    "INSERT OR REPLACE INTO UserType "
    "(ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, "
    "MultiSessions) VALUES (100, 7, 29, 'svc', 'svc', 3, 1);";
const std::string_view kSvcUser = "svc";
const std::string_view kSvcPassword = "svc-e2e-password";

const std::string_view kHistorizeSimulatedItemSql =
    "UPDATE AnalogItemType SET Simulated=1, HasHistoricalDatabaseNS=6, "
    "HasHistoricalDatabaseID=2 WHERE ID=4;";

const std::string_view kHistorizedNodeId = "ns=2;i=4";

const std::string_view kStripDataItemRowsSql =
    "PRAGMA foreign_keys=OFF;\n"
    "DELETE FROM AnalogItemType;\n"
    "DELETE FROM DiscreteItemType;\n";

namespace {

constexpr auto kClusterStartTimeout = 30s;

// The file-instance namespace the proxy claims to the file store, and the
// FileSystem root object it anchors top-level AddNodes with. See
// ConfigureProxyRole / ServerCluster::AggregationServers for why both are
// needed.
constexpr std::string_view kFileTypeNamespaceUri =
    "http://telecontrol.ru/opcua/filesystem/FileType";
constexpr std::string_view kFileSystemRootNodeId = "ns=7;i=304";

boost::json::object& EnsureObject(boost::json::object& parent,
                                  std::string_view key) {
  auto& value = parent[key];
  return value.is_object() ? value.as_object() : value.emplace_object();
}

// Turns off namespace/node permission enforcement when a suite asked for it.
// Absent from the config (the default) leaves the framework default in place.
void ApplyPermissionEnforcement(boost::json::object& server_json,
                                bool enforce_permissions) {
  if (enforce_permissions)
    return;
  EnsureObject(server_json, "security")["enforcePermissions"] = false;
}

// Drops every protocol driver block, so a tier that serves configuration,
// history, files or aggregation runs no device I/O of its own.
void EraseDrivers(boost::json::object& server_json) {
  server_json.erase("iec60870");
  server_json.erase("modbus");
  server_json.erase("iec61850");
}

}  // namespace

void ProvisionSvcPassword(boost::json::object& server_json) {
  auto& security = EnsureObject(server_json, "security");
  auto& provision = security["provision"].is_array()
                        ? security["provision"].as_array()
                        : security["provision"].emplace_array();
  provision.push_back(boost::json::object{
      {"id", 100}, {"password", std::string{kSvcPassword}}});
}

namespace {

std::filesystem::path GetSignedLicensePath() {
  auto* value = std::getenv("SCADA_SERVER_LICENSE_FILE");
  if (!value || !*value)
    return {};

  std::error_code ec;
  auto absolute = std::filesystem::absolute(std::filesystem::path{value}, ec);
  return ec ? std::filesystem::path{value} : absolute;
}

}  // namespace

void ConfigureSignedLicenseFromEnv(boost::json::object& server_json) {
  const auto license_path = GetSignedLicensePath();
  if (license_path.empty())
    return;

  boost::json::object license{{"file", license_path.string()}};

  if (auto* require_gcp_binding =
          std::getenv("SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING");
      require_gcp_binding && *require_gcp_binding) {
    license["require_gcp_binding"] =
        std::string_view{require_gcp_binding} == "true" ||
        std::string_view{require_gcp_binding} == "1";
  }

  server_json["license"] = std::move(license);
}

::testing::AssertionResult ValidateSignedLicenseEnv() {
  const auto license_path = GetSignedLicensePath();
  if (license_path.empty()) {
    return ::testing::AssertionFailure()
           << "SCADA_SERVER_LICENSE_FILE must be set to an external signed "
              "license JSON before running E2E tests";
  }
  if (!std::filesystem::exists(license_path)) {
    return ::testing::AssertionFailure()
           << "SCADA_SERVER_LICENSE_FILE points to missing license file: "
           << license_path;
  }
  return ::testing::AssertionSuccess();
}

void ConfigureTelemetry(boost::json::object& server_json,
                        std::string_view service_name,
                        std::string_view endpoint) {
  if (endpoint.empty())
    return;

  // service_name/endpoint are shared by the metric and trace exporters (one
  // OTLP destination and resource identity per tier — see
  // scada-server-framework/docs/tracing.md). Ratio 1.0: an E2E run is a
  // handful of requests, and sampling any of them out would leave holes in
  // the very waterfall being inspected — or, for the service x namespace
  // sweep, drop the span that IS the assertion.
  server_json["metrics"] = boost::json::object{
      {"service_name", std::string{service_name}},
      {"endpoint", std::string{endpoint}},
      {"traces",
       boost::json::object{{"enabled", true}, {"sampling_ratio", 1.0}}}};

  // Structured log export is a nested block of the template's existing "log"
  // object (which carries the file-sink dir/rotation) — merge, don't replace.
  auto& log = EnsureObject(server_json, "log");
  log["otlp"] = boost::json::object{{"enabled", true},
                                    {"service_name", std::string{service_name}},
                                    {"endpoint", std::string{endpoint}},
                                    {"min_severity", "info"}};
}

std::string_view ToString(ClusterTier tier) {
  switch (tier) {
    case ClusterTier::kConfig:
      return "config";
    case ClusterTier::kHistorian:
      return "historian";
    case ClusterTier::kIec104:
      return "iec104";
    case ClusterTier::kModbus:
      return "modbus";
    case ClusterTier::kIec61850:
      return "iec61850";
    case ClusterTier::kFilesystem:
      return "filesystem";
    case ClusterTier::kOpc:
      return "opc";
    case ClusterTier::kVidicon:
      return "vidicon";
  }
  return {};
}

std::string TelemetryServiceName(ClusterTier tier) {
  // The iec104 tier reports under its served GROUP name, not its binary name —
  // its driver block (and therefore the name the existing suites derive) is
  // "iec60870". See docs/server/tier-namespace-map.md, "Tier name != group
  // name".
  const std::string_view name = tier == ClusterTier::kIec104
                                    ? std::string_view{"iec60870"}
                                    : ToString(tier);
  return "scada-e2e-" + std::string{name};
}

// One member of the cluster: its tier identity, binary, and the process once
// launched.
class ServerCluster::Impl {
 public:
  Impl(ClusterExecutables executables, MakeContextFn make_context)
      : executables_{std::move(executables)},
        make_context_{std::move(make_context)} {}

  ::testing::AssertionResult Start(PortPool& ports,
                                   const ClusterOptions& options);

  ServerTier* Tier(ClusterTier tier) const {
    auto it = tiers_.find(tier);
    return it == tiers_.end() ? nullptr : it->second.get();
  }

  std::vector<ServerTier*> TiersForShutdown() const {
    // Edges and module tiers first, then the file store, then the
    // config/historian everything else depends on.
    static constexpr ClusterTier kOrder[] = {
        ClusterTier::kIec104,    ClusterTier::kModbus,
        ClusterTier::kIec61850,  ClusterTier::kOpc,
        ClusterTier::kVidicon,   ClusterTier::kFilesystem,
        ClusterTier::kHistorian, ClusterTier::kConfig};
    std::vector<ServerTier*> result;
    for (ClusterTier tier : kOrder) {
      if (ServerTier* launched = Tier(tier))
        result.push_back(launched);
    }
    return result;
  }

  boost::json::array AggregationServers() const { return aggregation_servers_; }

 private:
  // Creates a tier's ServerTier and allocates its ports, without launching it.
  // Split from launching because the edges' and historian's configuration
  // reference each other's URLs, which are only known once ports are allocated.
  ServerTier& Reserve(ClusterTier tier,
                      const std::filesystem::path& exe,
                      PortPool& ports);

  ClusterExecutables executables_;
  MakeContextFn make_context_;
  std::map<ClusterTier, std::unique_ptr<ServerTier>> tiers_;
  boost::json::array aggregation_servers_;
};

ServerTier& ServerCluster::Impl::Reserve(ClusterTier tier,
                                         const std::filesystem::path& exe,
                                         PortPool& ports) {
  auto launched = std::make_unique<ServerTier>(make_context_(exe));
  launched->AllocatePorts(ports);
  ServerTier& ref = *launched;
  tiers_.emplace(tier, std::move(launched));
  return ref;
}

::testing::AssertionResult ServerCluster::Impl::Start(
    PortPool& ports,
    const ClusterOptions& options) {
  const std::string_view endpoint = options.otlp_endpoint;
  const bool enforce_permissions = options.enforce_permissions;

  // --- Config tier -----------------------------------------------------------
  // Owns the configuration namespace (devices, data items, users, filesystem)
  // that the edges read and re-expose through aggregation. Keeps its generated
  // local config DB plus the svc account the edges authenticate with, and
  // serves the per-protocol device config. The IEC 61850 device port (the
  // shared test server) and the TIT.4 historization live here because the edges
  // read their config from here. Mirrors
  // gcp/free-tier/multitier/configs/config.json.
  ServerTier& config =
      Reserve(ClusterTier::kConfig, executables_.config, ports);
  std::string config_sql{kSvcUserSql};
  if (options.historize_simulated_item)
    config_sql += std::string{kHistorizeSimulatedItemSql};
  config.Launch(ServerTier::Options{
      .configure =
          [endpoint, enforce_permissions](boost::json::object& json) {
            json["deviceConfig"] = boost::json::object{};
            // Serves config only — no protocol drivers of its own, and no file
            // store (the filesystem tier owns the FileSystem subtree).
            EraseDrivers(json);
            json.erase("filesystem");
            ProvisionSvcPassword(json);
            ConfigureTelemetry(json, TelemetryServiceName(ClusterTier::kConfig),
                               endpoint);
            ApplyPermissionEnforcement(json, enforce_permissions);
          },
      .iec61850_port = options.iec61850_port,
      .extra_config_sql = config_sql,
  });
  if (!config.WaitListening()) {
    return ::testing::AssertionFailure()
           << "cluster config tier did not start listening on OPC UA port "
           << config.opcua_port();
  }

  // --- Historian tier (reserved now; launched below once the edges' ports are
  //     known so it can pull-collect from one) --------------------------------
  // Owns the history store. In the history test it pull-collects the historized
  // TIT.4 from the edge that serves it (historyCollection.sources — the ADR
  // 0002 subscription model) and files the samples under its own
  // HasHistoricalDatabase config. It self-registers with the proxy via OPC UA
  // RegisterServer2 advertising the "HD" capability, and the proxy's
  // history-link module routes ALL client HistoryRead/HistoryUpdate to it —
  // the historian is deliberately NOT an aggregation downstream. Mirrors
  // gcp/free-tier/multitier/configs/historian.json.
  ServerTier& historian =
      Reserve(ClusterTier::kHistorian, executables_.historian, ports);
  const std::string config_url = config.OpcUaUrl();
  const std::string historian_url = historian.OpcUaUrl();

  // --- Device edges (reserved now; launched after the historian so its
  //     historyCollection can reference an edge's already-known URL) ----------
  // Each edge is a config client (no local DB) running exactly one driver,
  // fetching config from the config tier as svc. Edges run NO history module
  // (ADR 0002): the historian pull-collects from them, and clients read
  // history back through the proxy's history link to the historian.
  // The proxy aggregates the edges anonymously. Mirrors the GCP
  // configs/{iec104,modbus,iec61850}.json edges.
  //
  // The modbus edge is aggregated DYNAMICALLY: instead of a static
  // aggregation.servers entry it self-registers with the proxy via OPC UA
  // RegisterServer (WS-F: opcua.register_with_url + advertise_url + a unique
  // application_uri), and the proxy's DiscoveryRegistry reconcile loop stands
  // the downstream up. This keeps permanent E2E coverage of the discovery
  // path the Windows on-prem deployment wires edges with, while iec104 /
  // iec61850 keep covering the static path.
  struct EdgeSpec {
    ClusterTier tier;
    std::filesystem::path exe;
    std::string_view driver;
    bool dynamic_registration = false;
  };
  const EdgeSpec edges[] = {
      {ClusterTier::kIec104, executables_.iec104, "iec60870"},
      {ClusterTier::kModbus, executables_.modbus, "modbus",
       /*dynamic_registration=*/true},
      {ClusterTier::kIec61850, executables_.iec61850, "iec61850"},
  };
  for (const EdgeSpec& edge : edges)
    Reserve(edge.tier, edge.exe, ports);

  // The iec104 edge serves the historized TIT.4; the historian pulls it from
  // there (any edge would do — all serve the config-derived data items).
  const std::string collect_source_url = Tier(ClusterTier::kIec104)->OpcUaUrl();

  std::string historian_sql{kSvcUserSql};
  if (options.historize_simulated_item)
    historian_sql += std::string{kHistorizeSimulatedItemSql};
  historian.Launch(ServerTier::Options{
      .configure =
          [collect_source_url, historian_url, endpoint, enforce_permissions,
           proxy_opcua_url = options.proxy_opcua_url,
           historize =
               options.historize_simulated_item](boost::json::object& json) {
            EraseDrivers(json);
            json.erase("filesystem");
            ProvisionSvcPassword(json);
            ConfigureTelemetry(
                json, TelemetryServiceName(ClusterTier::kHistorian), endpoint);
            ApplyPermissionEnforcement(json, enforce_permissions);
            // The historian self-registers with the proxy via OPC UA
            // RegisterServer2, advertising the "HD" (Historical Data)
            // ServerCapabilityIdentifier (Part 4 §5.4.6, Part 12 Annex D).
            // The proxy's history-link module links an HD registrant's
            // history services (and its aggregation reconcile skips it — a
            // historian owns no address-space namespaces). This is the
            // discovery path the Windows on-prem deployment wires the
            // historian with.
            auto& opcua = json.at("opcua").as_object();
            opcua["application_uri"] = "urn:e2e:scada:historian";
            opcua["advertise_url"] = historian_url;
            opcua["register_with_url"] = proxy_opcua_url;
            opcua["server_capabilities"] = boost::json::array{"HD"};
            if (historize) {
              json["historyCollection"] = boost::json::object{
                  {"sources", boost::json::array{boost::json::object{
                                  {"endpoint", collect_source_url},
                                  {"user", std::string{kSvcUser}},
                                  {"password", std::string{kSvcPassword}},
                                  {"nodes", boost::json::array{std::string{
                                                kHistorizedNodeId}}}}}}};
            }
          },
      .extra_config_sql = historian_sql,
  });
  if (!historian.WaitListening()) {
    return ::testing::AssertionFailure()
           << "cluster historian tier did not start listening on OPC UA port "
           << historian.opcua_port();
  }

  auto make_edge_configure = [config_url, endpoint, enforce_permissions,
                              proxy_opcua_url = options.proxy_opcua_url](
                                 std::string_view keep_driver,
                                 bool dynamic_registration,
                                 std::string advertise_url) {
    return [config_url, proxy_opcua_url, endpoint, enforce_permissions,
            keep_driver, dynamic_registration,
            advertise_url =
                std::move(advertise_url)](boost::json::object& json) {
      for (std::string_view driver : {"iec60870", "modbus", "iec61850"}) {
        if (driver != keep_driver)
          json.erase(driver);
      }
      // The filesystem tier exclusively owns the FileSystem subtree; an edge
      // running its own file store would merge a second tree into the proxy's
      // fan-out Browse.
      json.erase("filesystem");
      json["configuration"] =
          boost::json::object{{"endpoint", config_url},
                              {"user", std::string{kSvcUser}},
                              {"password", std::string{kSvcPassword}}};
      // Edge binaries link no history module (ADR 0002) — a "history" block
      // here would be dead config, so drop the template's.
      json.erase("history");
      ConfigureTelemetry(json, "scada-e2e-" + std::string{keep_driver},
                         endpoint);
      ApplyPermissionEnforcement(json, enforce_permissions);
      if (dynamic_registration) {
        // WS-F self-registration: a per-edge application_uri (the registry
        // keys registrations by server URI) and an externally-reachable
        // advertise_url the proxy connects back on.
        auto& opcua = json.at("opcua").as_object();
        opcua["application_uri"] = "urn:e2e:scada:" + std::string{keep_driver};
        opcua["advertise_url"] = advertise_url;
        opcua["register_with_url"] = proxy_opcua_url;
      }
    };
  };
  for (const EdgeSpec& edge : edges) {
    ServerTier* launched = Tier(edge.tier);
    launched->Launch(ServerTier::Options{
        .configure = make_edge_configure(edge.driver, edge.dynamic_registration,
                                         launched->OpcUaUrl()),
        .remove_local_config_db = true,
    });
  }
  for (const EdgeSpec& edge : edges) {
    ServerTier* launched = Tier(edge.tier);
    if (!launched->WaitListening()) {
      return ::testing::AssertionFailure()
             << "cluster " << edge.driver
             << " edge did not start listening on OPC UA port "
             << launched->opcua_port();
    }
  }

  // --- Filesystem tier -------------------------------------------------------
  // The dedicated file store: serves the FileSystem subtree from its own
  // workspace; no drivers/history/data items. It keeps a LOCAL config DB (like
  // config/historian/proxy) because it must authenticate the proxy's svc
  // aggregation login — anonymous sessions are denied the forwarded
  // AddNodes/DeleteNodes, and remote-config tiers cannot resolve non-root
  // users yet (the known LoadNodes(UserType) gap). Its data-item rows are
  // stripped like the proxy's so the edges stay authoritative for values. The
  // proxy exclusively claims the file namespace + root to it below. Mirrors
  // gcp/free-tier/multitier/configs/filesystem.json.
  ServerTier& file_store =
      Reserve(ClusterTier::kFilesystem, executables_.filesystem, ports);
  file_store.Launch(ServerTier::Options{
      .configure =
          [endpoint, enforce_permissions](boost::json::object& json) {
            EraseDrivers(json);
            json["dataItems"] = boost::json::object{{"enabled", false}};
            // No history module in the filesystem tier binary; drop the dead
            // template block.
            json.erase("history");
            ProvisionSvcPassword(json);
            ConfigureTelemetry(
                json, TelemetryServiceName(ClusterTier::kFilesystem), endpoint);
            ApplyPermissionEnforcement(json, enforce_permissions);
            // The template's filesystem block stays: it roots the store at this
            // tier's own ${DIR_PARAM}/FileSystem workspace dir.
          },
      .extra_config_sql =
          std::string{kSvcUserSql} + std::string{kStripDataItemRowsSql},
  });
  if (!file_store.WaitListening()) {
    return ::testing::AssertionFailure()
           << "cluster filesystem tier did not start listening on OPC UA port "
           << file_store.opcua_port();
  }

  // --- Windows-only module tiers ---------------------------------------------
  // Classic OPC (ns=25) and Vidicon (ns=28, 29). Both own a tier-exclusive
  // namespace group, so the proxy routes them by ownership with no claim.
  // Neither needs its backing system present: the Classic OPC client starts
  // with no configured servers, and the Vidicon loaders degrade to an empty
  // tree when no Vidicon installation is found (both are what their repos'
  // startup E2Es pin). They keep a local config DB to authenticate the proxy's
  // svc login, with data-item rows stripped so the edges stay authoritative.
#if defined(_WIN32)
  if (options.include_windows_module_tiers) {
    struct ModuleTierSpec {
      ClusterTier tier;
      std::filesystem::path exe;
      // The module block this tier exists to serve (its whole config value —
      // the OPC tier is driven by an "opc.client" block, the Vidicon tier by a
      // plain enable), and the sibling module block to disable: each binary
      // links both, and only one runs.
      std::string_view enable;
      boost::json::object enable_value;
      std::string_view disable;
    };
    const ModuleTierSpec module_tiers[] = {
        {ClusterTier::kOpc, executables_.opc, "opc",
         boost::json::object{{"client", boost::json::object{}}}, "vidicon"},
        {ClusterTier::kVidicon, executables_.vidicon, "vidicon",
         boost::json::object{{"enabled", true}}, "opc"},
    };
    for (const ModuleTierSpec& spec : module_tiers) {
      ServerTier& module_tier = Reserve(spec.tier, spec.exe, ports);
      module_tier.Launch(ServerTier::Options{
          .configure =
              [endpoint, enforce_permissions,
               &spec](boost::json::object& json) {
                EraseDrivers(json);
                json.erase("filesystem");
                json.erase("history");
                json["dataItems"] = boost::json::object{{"enabled", false}};
                json[spec.enable] = spec.enable_value;
                json[spec.disable] = boost::json::object{{"enabled", false}};
                ProvisionSvcPassword(json);
                ConfigureTelemetry(json, TelemetryServiceName(spec.tier),
                                   endpoint);
                ApplyPermissionEnforcement(json, enforce_permissions);
              },
          .extra_config_sql =
              std::string{kSvcUserSql} + std::string{kStripDataItemRowsSql},
      });
      if (!module_tier.WaitListening()) {
        return ::testing::AssertionFailure()
               << "cluster " << ToString(spec.tier)
               << " tier did not start listening on OPC UA port "
               << module_tier.opcua_port();
      }
    }
  }
#endif

  // --- Aggregation entries for the caller's proxy
  // -----------------------------
  for (const EdgeSpec& edge : edges) {
    // Dynamically-registered edges have no static entry — they arrive through
    // the DiscoveryRegistry.
    if (edge.dynamic_registration)
      continue;
    // forward_events mirrors gcp/free-tier/multitier/configs/proxy.json, where
    // every edge re-raises its process/alarm/device events to proxy clients.
    // It is not decoration here: the tap is a second, permanently-waiting
    // consumer of the downstream session's subscription, and while consumers
    // shared one notification queue it drained the data changes belonging to
    // client subscriptions. Leaving it off made this fixture the one topology
    // where aggregated live values worked.
    aggregation_servers_.push_back(boost::json::object{
        {"endpoint", Tier(edge.tier)->OpcUaUrl()}, {"forward_events", true}});
  }
  // The file-store downstream. The "namespaces" claim names the tier-exclusive
  // file-instance namespace (FILESYSTEM_FILE). Beyond routing that namespace
  // here, the claim is what SCOPES this downstream: a downstream with any
  // namespace claim routes single-target services (monitored items, Write,
  // Call, NodeManagement) for ONLY its claimed namespaces
  // (RemoteNodeManager::has_namespace_claims → ClaimsProxyNamespace). Without
  // it, the file store falls back to routing every namespace its NamespaceArray
  // still publishes — which includes the shared "dedicated" data-item
  // namespaces every tier keeps (e.g. ns=2 TIT) even though the file store
  // deleted its data-item rows, so it could capture an edge item's monitored
  // items or writes. The extra "nodes" claim for the FileSystem root object
  // i=304 lives in the shared SCADA namespace (every tier serves it) and so
  // cannot be routed by namespace alone — it anchors top-level AddNodes to
  // this tier. Model-change events are re-raised to the proxy's clients; the
  // link presents svc, since file create/delete forwarding needs a
  // non-anonymous downstream session under enforce_permissions. Mirrors the
  // GCP proxy.json entry.
  aggregation_servers_.push_back(boost::json::object{
      {"endpoint", file_store.OpcUaUrl()},
      {"user", std::string{kSvcUser}},
      {"password", std::string{kSvcPassword}},
      {"namespaces", boost::json::array{std::string{kFileTypeNamespaceUri}}},
      {"nodes", boost::json::array{std::string{kFileSystemRootNodeId}}},
      {"forward_events", true}});
  for (ClusterTier tier : {ClusterTier::kOpc, ClusterTier::kVidicon}) {
    // Exclusive namespace groups: routed by ownership, so no claim entry.
    if (ServerTier* launched = Tier(tier)) {
      aggregation_servers_.push_back(
          boost::json::object{{"endpoint", launched->OpcUaUrl()},
                              {"user", std::string{kSvcUser}},
                              {"password", std::string{kSvcPassword}}});
    }
  }

  return ::testing::AssertionSuccess();
}

ServerCluster::ServerCluster(ClusterExecutables executables,
                             MakeContextFn make_context)
    : impl_{std::make_unique<Impl>(std::move(executables),
                                   std::move(make_context))} {}

ServerCluster::~ServerCluster() = default;

::testing::AssertionResult ServerCluster::Start(PortPool& ports,
                                                const ClusterOptions& options) {
  return impl_->Start(ports, options);
}

ServerTier* ServerCluster::Tier(ClusterTier tier) const {
  return impl_->Tier(tier);
}

std::vector<ServerTier*> ServerCluster::TiersForShutdown() const {
  return impl_->TiersForShutdown();
}

boost::json::array ServerCluster::AggregationServers() const {
  return impl_->AggregationServers();
}

void ServerCluster::PreserveWorkspaces() {
  for (ServerTier* tier : impl_->TiersForShutdown())
    tier->PreserveWorkspace();
}

void ServerCluster::Terminate() {
  for (ServerTier* tier : impl_->TiersForShutdown())
    tier->Terminate();
}

void ConfigureProxyRole(boost::json::object& server_json,
                        const ProxyRoleOptions& options) {
  EraseDrivers(server_json);
  // The filesystem tier owns the FileSystem subtree; the proxy must not run a
  // local file store of its own. The proxy binary also links no history module
  // — history is the history-link module's (below).
  server_json.erase("filesystem");
  server_json.erase("history");
  server_json["dataItems"] = boost::json::object{{"enabled", false}};
  if (options.aggregation_servers) {
    server_json["aggregation"] =
        boost::json::object{{"servers", *options.aggregation_servers}};
  }
  // History lives in the historian tier, not in the namespace-owning edges
  // (edges run no history module), so the proxy routes ALL
  // HistoryRead/HistoryUpdate through the history-link module. No endpoint: the
  // link is discovery-driven — it links the RegisterServer2 registrant
  // advertising "HD". The link presents svc so the historian's permission
  // enforcement accepts the forwarded reads.
  server_json["historyLink"] = boost::json::object{
      {"user", std::string{kSvcUser}}, {"password", std::string{kSvcPassword}}};
  ConfigureTelemetry(server_json, options.service_name, options.otlp_endpoint);
  ApplyPermissionEnforcement(server_json, options.enforce_permissions);
}

::testing::AssertionResult WaitForProxyDownstreams(
    const std::filesystem::path& proxy_log_dir) {
  const auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
      kClusterStartTimeout);

  if (!WaitUntil(
          [&proxy_log_dir] {
            return ContainsInDirectory(proxy_log_dir,
                                       "Aggregating registered downstream");
          },
          timeout)) {
    return ::testing::AssertionFailure()
           << "the proxy never aggregated the RegisterServer-registered modbus "
              "edge; see the proxy log in "
           << proxy_log_dir;
  }

  if (!WaitUntil(
          [&proxy_log_dir] {
            return ContainsInDirectory(proxy_log_dir, "Linked historian");
          },
          timeout)) {
    return ::testing::AssertionFailure()
           << "the proxy never linked the HD-registered historian; see the "
              "proxy log in "
           << proxy_log_dir;
  }

  return ::testing::AssertionSuccess();
}

}  // namespace client::test
