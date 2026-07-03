#pragma once

#include "test/e2e/e2e_file_helpers.h"
#include "test/e2e/e2e_process.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace boost::json {
class object;
}

namespace client::test {

// Everything the server-process harness needs that differs per test target: the
// binary and fixture paths (each E2E target injects its own compile-time
// `SCADA_E2E_*` values) and how a signed license is applied to a server.json
// object (the server integration suite embeds a test license, the client suite
// points at an external `SCADA_SERVER_LICENSE_FILE`). Keeping these injected
// lets the harness itself stay free of target-specific macros and license
// policy, so it can be compiled into both the server and client E2E targets.
struct ServerProcessContext {
  std::filesystem::path server_exe;
  std::filesystem::path fixture_dir;
  std::filesystem::path settings_template;
  std::filesystem::path configuration_base_sql;
  std::filesystem::path configuration_fixture_sql;
  std::filesystem::path sqlite_exe;
  // Applies the target's license to a server.json object; `workspace` is the
  // server's workspace (some licenses are written into it).
  std::function<void(boost::json::object& server_json,
                     const std::filesystem::path& workspace)>
      configure_license;
};

// Generates a fresh configuration SQLite DB under `workspace`/Configuration
// from the base + fixture SQL in `context`, optionally rewriting the IEC 61850
// device port. Throws std::runtime_error if the sqlite3 tool fails.
void GenerateConfigurationDatabase(
    const ServerProcessContext& context,
    const std::filesystem::path& workspace,
    std::optional<int> iec61850_port = std::nullopt);

// Runs `sql` against `workspace`'s configuration DB (e.g. to strip data items
// or add a service account). Throws std::runtime_error if the sqlite3 tool
// fails.
void ExecuteConfigurationSql(const ServerProcessContext& context,
                             const std::filesystem::path& workspace,
                             std::string_view sql);

// Hands out distinct TCP ports so the independent server processes of one test
// never collide. Not thread-safe; drive it from the test thread.
class PortPool {
 public:
  // Returns a free TCP port distinct from every port previously returned or
  // reserved on this pool.
  int Allocate();

  // Records an externally chosen port as used, so later Allocate() avoids it
  // (e.g. ports a fixture allocated for its own built-in process slots).
  void Reserve(int port);

 private:
  std::set<int> used_;
};

// One server process of a multi-tier integration cluster: its own temp
// workspace, generated config DB, job/child process, and OPC UA +
// remote-session ports. Lets a test stand up as many independent server
// processes as a topology needs — the config / historian / edge / proxy split
// from server/dev/local-cluster. Owns a JobObject/TempWorkspace, so it is
// non-copyable and non-movable; hold instances via unique_ptr.
class ServerTier {
 public:
  // Shapes a tier before launch. `configure` mutates the server.json object to
  // give the tier its role (drivers on/off, aggregation, remote config/history
  // endpoints, ...). `iec61850_port` seeds the tier's device config so an edge
  // instantiated from it polls the shared IEC 61850 test server.
  // `extra_config_sql` runs against the generated config DB (e.g. historize a
  // node, add a multi-session service account). `remove_local_config_db`
  // deletes the generated DB so an edge provably holds no config of its own —
  // the remote-config path.
  struct Options {
    std::function<void(boost::json::object&)> configure;
    std::optional<int> iec61850_port;
    std::string extra_config_sql;
    bool remove_local_config_db = false;
  };

  explicit ServerTier(ServerProcessContext context);
  ServerTier(const ServerTier&) = delete;
  ServerTier& operator=(const ServerTier&) = delete;

  int opcua_port() const { return opcua_port_; }
  int remote_port() const { return remote_port_; }

  // The tier's OPC UA endpoint URL (opc.tcp://127.0.0.1:<opcua_port>).
  std::string OpcUaUrl() const;

  // The tier's workspace root and its Logs subdirectory (for reading the
  // tier's config DB and for ContainsInDirectory log assertions).
  const std::filesystem::path& WorkspaceDir() const;
  std::filesystem::path LogDir() const;

  bool IsRunning() const { return process_.IsRunning(); }

  // Allocates this tier's OPC UA + remote-session ports from `pool`. Call once,
  // before Launch().
  void AllocatePorts(PortPool& pool);

  // Prepares the workspace (fixture copy + fresh config DB) and server.json
  // from the shared template with this tier's ports, applies `options`, and
  // launches the server exe. Does not wait for a listener; call WaitListening()
  // (or pick another readiness signal) next. Requires AllocatePorts() first.
  void Launch(const Options& options);

  // Blocks until the OPC UA listener accepts a connection, up to an internal
  // start timeout. Returns false on timeout.
  bool WaitListening() const;

  // Terminates the process (idempotent).
  void Terminate();

  // Keeps the workspace on disk after the test for post-mortem inspection.
  void PreserveWorkspace() { workspace_.Preserve(); }

 private:
  ServerProcessContext context_;
  TempWorkspace workspace_;
  JobObject job_;
  ChildProcess process_;
  int opcua_port_ = 0;
  int remote_port_ = 0;
};

}  // namespace client::test
