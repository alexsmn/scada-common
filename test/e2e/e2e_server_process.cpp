#include "test/e2e/e2e_server_process.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/start_dir.hpp>

#include <chrono>
#include <stdexcept>
#include <system_error>
#include <thread>

using namespace std::chrono_literals;

namespace client::test {
namespace {

constexpr auto kWaitStep = 100ms;
constexpr auto kServerStartTimeout = 30s;

template <class Predicate>
bool WaitUntil(Predicate&& predicate,
               std::chrono::milliseconds timeout,
               std::chrono::milliseconds step = kWaitStep) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate())
      return true;
    std::this_thread::sleep_for(step);
  }
  return predicate();
}

std::string SqlitePath(const std::filesystem::path& path) {
  auto result = path.lexically_normal().generic_string();
  for (auto& ch : result) {
    if (ch == '\'')
      ch = ' ';
  }
  return result;
}

// Runs the sqlite3 tool with `-init script` against `database_path`. Throws
// std::runtime_error (with `action` and sqlite3's own diagnostics in the
// message) on a non-zero exit.
void RunSqliteScript(const ServerProcessContext& context,
                     const std::filesystem::path& workspace,
                     const std::filesystem::path& database_path,
                     const std::filesystem::path& script_path,
                     std::string_view action) {
  // sqlite3 names the rejected statement and the reason ("no such column",
  // "database is locked", ...) on stderr; capture both streams to a file beside
  // the script so a failure is diagnosable from the message alone, and so the
  // text survives in preserved workspaces.
  const auto output_path =
      std::filesystem::path{script_path}.concat(".output.txt");

  JobObject job;
  ChildProcess sqlite;
  try {
    sqlite.process = process::child{
        context.sqlite_exe.string(),
        process::args(
            {"-batch", "-init", script_path.string(), database_path.string()}),
        process::start_dir(workspace.string()),
        // sqlite3 keeps reading SQL from stdin after the -init script has run.
        // Inheriting the test binary's stdin lets whatever happens to sit there
        // (terminal input, a piped harness script) be executed as SQL, which
        // `.bail on` then turns into a spurious exit-1 "failure" of the fixture
        // SQL. Feed it EOF instead so only the script runs.
        (process::std_in < process::null),
        ((process::std_out & process::std_err) > output_path.string()),
        job.group()};
  } catch (const std::system_error& e) {
    throw std::runtime_error{"Failed to launch " + context.sqlite_exe.string() +
                             ": " + e.what()};
  }

  WaitForExit(sqlite, 30000);
  auto exit_code = sqlite.ExitCode();
  if (!exit_code || *exit_code != 0) {
    ForceTerminate(sqlite);
    std::string diagnostics = ReadFileOrEmpty(output_path);
    while (!diagnostics.empty() &&
           (diagnostics.back() == '\n' || diagnostics.back() == '\r'))
      diagnostics.pop_back();
    throw std::runtime_error{
        "sqlite3 failed while " + std::string{action} + " " +
        database_path.string() + " with exit code " +
        (exit_code ? std::to_string(*exit_code) : std::string{"unavailable"}) +
        (diagnostics.empty() ? std::string{": (no sqlite3 output)"}
                             : ": " + diagnostics) +
        "\nscript: " + script_path.string()};
  }
}

}  // namespace

int PortPool::Allocate() {
  boost::asio::io_context io_context;
  const auto find_port = [&io_context] {
    boost::asio::ip::tcp::acceptor acceptor{
        io_context,
        boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    return static_cast<int>(acceptor.local_endpoint().port());
  };
  int port = find_port();
  while (used_.count(port))
    port = find_port();
  used_.insert(port);
  return port;
}

void PortPool::Reserve(int port) {
  used_.insert(port);
}

namespace {

bool CanConnectTcp(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver{io_context};
    boost::asio::ip::tcp::socket socket{io_context};
    auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
    boost::asio::connect(socket, endpoints);
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

void GenerateConfigurationDatabase(const ServerProcessContext& context,
                                   const std::filesystem::path& workspace,
                                   std::optional<int> iec61850_port) {
  const auto configuration_dir = workspace / "Configuration";
  const auto database_path = configuration_dir / "configuration.sqlite3";
  const auto script_path = workspace / "generate-configuration.sql";
  std::error_code ec;
  std::filesystem::remove_all(configuration_dir, ec);
  std::filesystem::create_directories(configuration_dir, ec);

  auto script =
      ".bail on\n"
      ".read " +
      SqlitePath(context.configuration_base_sql) +
      "\n"
      ".read " +
      SqlitePath(context.configuration_fixture_sql) + "\n";
  if (iec61850_port) {
    script += "UPDATE Iec61850DeviceType SET Port = " +
              std::to_string(*iec61850_port) + ";\n";
  }
  WriteTextFile(script_path, script);
  RunSqliteScript(context, workspace, database_path, script_path, "creating");
}

void ExecuteConfigurationSql(const ServerProcessContext& context,
                             const std::filesystem::path& workspace,
                             std::string_view sql) {
  const auto database_path =
      workspace / "Configuration" / "configuration.sqlite3";
  const auto script_path = workspace / "execute-configuration.sql";
  std::string script = ".bail on\n";
  script += sql;
  script += "\n";
  WriteTextFile(script_path, script);
  RunSqliteScript(context, workspace, database_path, script_path, "updating");
}

ServerTier::ServerTier(ServerProcessContext context)
    : context_{std::move(context)} {}

std::string ServerTier::OpcUaUrl() const {
  return "opc.tcp://127.0.0.1:" + std::to_string(opcua_port_);
}

const std::filesystem::path& ServerTier::WorkspaceDir() const {
  return workspace_.path();
}

std::filesystem::path ServerTier::LogDir() const {
  return workspace_.path() / "Logs";
}

void ServerTier::AllocatePorts(PortPool& pool) {
  opcua_port_ = pool.Allocate();
  remote_port_ = pool.Allocate();
}

void ServerTier::Launch(const Options& options) {
  std::filesystem::copy(context_.fixture_dir, workspace_.path(),
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);
  GenerateConfigurationDatabase(context_, workspace_.path(),
                                options.iec61850_port);
  if (!options.extra_config_sql.empty())
    ExecuteConfigurationSql(context_, workspace_.path(),
                            options.extra_config_sql);
  if (options.remove_local_config_db) {
    std::error_code ec;
    std::filesystem::remove(
        workspace_.path() / "Configuration" / "configuration.sqlite3", ec);
  }

  auto json_value =
      boost::json::parse(ReadFileOrEmpty(context_.settings_template));
  auto& json = json_value.as_object();
  if (context_.configure_license)
    context_.configure_license(json, workspace_.path());
  json["metrics"] = boost::json::object{};
  json["sessions"] = boost::json::array{"tcp;passive;host=0.0.0.0;port=" +
                                        std::to_string(remote_port_)};
  auto& opcua = json["opcua"].is_object() ? json["opcua"].as_object()
                                          : json["opcua"].emplace_object();
  opcua["enabled"] = true;
  opcua["url"] =
      boost::json::array{"opc.tcp://127.0.0.1:" + std::to_string(opcua_port_)};
  opcua["trace"] = "none";
  if (options.configure)
    options.configure(json);

  WriteTextFile(workspace_.path() / "server.json",
                boost::json::serialize(json_value));
  LaunchProcess(context_.server_exe,
                {"--param=" + (workspace_.path() / "server.json").string()},
                workspace_.path(), job_, process_);
}

bool ServerTier::WaitListening() const {
  const int port = opcua_port_;
  return WaitUntil([port] { return CanConnectTcp(port); },
                   std::chrono::duration_cast<std::chrono::milliseconds>(
                       kServerStartTimeout));
}

void ServerTier::Terminate() {
  job_.Terminate();
  ForceTerminate(process_);
  WaitForExit(process_);
}

}  // namespace client::test
