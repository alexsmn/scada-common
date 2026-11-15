#include "model/namespace_uris.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace scada::model {
namespace {

TEST(ClassifyServerTest, OurCanonicalArrayIsOurs) {
  const std::vector<std::string> uris = GetCanonicalNamespaceUris();
  EXPECT_EQ(ClassifyServer(uris), ServerKind::kOurs);
}

// A tier publishes only the namespaces it serves (ADR 0003), at indexes of its
// own; the SCADA URI anywhere in the array is what makes it ours.
TEST(ClassifyServerTest, ATierSubsetAtOtherIndexesIsOurs) {
  const std::vector<std::string> uris = {std::string{kOpcUaNamespaceUri},
                                         "urn:some-host:scada-iec104",
                                         std::string{kScadaNamespaceUri}};
  EXPECT_EQ(ClassifyServer(uris), ServerKind::kOurs);
}

// What open62541's example server publishes: the standard namespace, its own
// application URI and one namespace of its own. Nothing of ours.
TEST(ClassifyServerTest, AThirdPartyArrayIsForeign) {
  const std::vector<std::string> uris = {std::string{kOpcUaNamespaceUri},
                                         "urn:open62541.server.application",
                                         "http://example.com/vendor"};
  EXPECT_EQ(ClassifyServer(uris), ServerKind::kForeign);
}

// Another telecontrol.ru URI is not the SCADA namespace: the decision is by
// exact URI, never by prefix.
TEST(ClassifyServerTest, ASiblingUriIsNotOurs) {
  const std::vector<std::string> uris = {
      std::string{kOpcUaNamespaceUri},
      "http://telecontrol.ru/opcua/scada/extra"};
  EXPECT_EQ(ClassifyServer(uris), ServerKind::kForeign);
}

}  // namespace
}  // namespace scada::model
