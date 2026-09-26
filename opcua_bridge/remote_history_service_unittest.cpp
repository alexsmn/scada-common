#include "opcua_bridge/remote_history_service.h"

#include "scada/locale_negotiation.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace scada::opcua_bridge {
namespace {

// The link to the historian is a TIER HOP, so it must ask for every language.
//
// One session here serves every client this server will ever answer. Asking
// for a single language makes the historian resolve on our behalf, and the
// other translations are gone before this server can choose per session —
// which is not recoverable downstream, because what arrives is a plain value
// that looks perfectly correct.
//
// Regression test with a measured failure behind it. Until 2026-09-21 these
// params carried no locale ids at all: the demo's archive held
// `mul|{"t":[["ru","Значение > 45"],["en","Value > 45"]]}` — both languages,
// correctly stored — and an English session was served «Значение > 45»,
// because the historian resolved against an empty list and returned the
// leading (configured) translation. Every test in the tree was green.
TEST(RemoteHistoryServiceTest, TheHistorianLinkAsksForEveryLanguage) {
  const RemoteHistoryServiceConfig config{.endpoint_url =
                                              "opc.tcp://historian:4843"};

  const opcua::SessionConnectParams params =
      MakeHistorySessionParams(config, config.endpoint_url);

  // The tier tag leads and "mul" follows, and the ORDER is the whole content of
  // this assertion. Only a leading entry licenses packing (Part 4 §5.4 gives
  // the special locales meaning in that position alone), and the trailing "mul"
  // is what an upstream predating the tag still recognises — so swapping them
  // silently gives up either the packed SourceName or the packed Message.
  // Backlog 819.
  EXPECT_EQ(params.locale_ids, (std::vector<std::string>{
                                   std::string{scada::kTierMultiLanguageLocale},
                                   std::string{scada::kMultiLanguageLocale}}));
}

// The endpoint and credentials still reach the session: a locale is an
// addition to them, never a replacement, and confusing the two would surface
// as an authentication failure rather than as a language one.
TEST(RemoteHistoryServiceTest, TheEndpointAndCredentialsAreCarried) {
  RemoteHistoryServiceConfig config;
  config.endpoint_url = "opc.tcp://historian:4843";
  config.user_name = u"svc";
  config.password = u"secret";

  const opcua::SessionConnectParams params =
      MakeHistorySessionParams(config, "opc.tcp://other:4843");

  EXPECT_EQ(params.connection_string, "opc.tcp://other:4843");
  EXPECT_EQ(params.user_name.text, u"svc");
  EXPECT_EQ(params.password.text, u"secret");
}

}  // namespace
}  // namespace scada::opcua_bridge
