#include "opcua_bridge/server_adapters.h"

#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include <optional>

namespace opcua_bridge {
namespace {

// A core ViewService that records the (converted) request it receives and
// returns a fixed result, so the adapter's both-directions conversion and
// delegation can be observed.
class FakeViewService : public scada::ViewService {
 public:
  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext,
      std::vector<scada::BrowseDescription> inputs) override {
    received_inputs = std::move(inputs);
    scada::BrowseResult result;
    result.status_code = scada::StatusCode::Good;
    result.references.push_back(
        scada::ReferenceDescription{.node_id = scada::NodeId{2253u}});
    co_return std::vector<scada::BrowseResult>{std::move(result)};
  }
  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath>) override {
    co_return std::vector<scada::BrowsePathResult>{};
  }

  std::vector<scada::BrowseDescription> received_inputs;
};

TEST(ServerAdapterTest, ViewServiceBrowseDelegatesAndConverts) {
  FakeViewService fake;
  ViewServiceAdapter adapter{fake};

  boost::asio::io_context io;
  std::optional<opcua::scada::StatusOr<std::vector<opcua::scada::BrowseResult>>>
      result;

  // Call the adapter through its opcua interface with opcua-typed input.
  std::vector<opcua::scada::BrowseDescription> inputs;
  inputs.push_back(opcua::scada::BrowseDescription{
      .node_id = opcua::scada::NodeId{84u},
      .direction = opcua::scada::BrowseDirection::Forward});

  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Browse(opcua::scada::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  // The adapter converted opcua input -> scada and delegated to the fake.
  ASSERT_EQ(fake.received_inputs.size(), 1u);
  EXPECT_EQ(fake.received_inputs[0].node_id, scada::NodeId{84u});
  EXPECT_EQ(fake.received_inputs[0].direction, scada::BrowseDirection::Forward);

  // The adapter converted the scada result back to opcua.
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  const auto& browse_result = (**result)[0];
  EXPECT_EQ(browse_result.status_code, opcua::scada::StatusCode::Good);
  ASSERT_EQ(browse_result.references.size(), 1u);
  EXPECT_EQ(browse_result.references[0].node_id, opcua::scada::NodeId{2253u});
}

}  // namespace
}  // namespace opcua_bridge
