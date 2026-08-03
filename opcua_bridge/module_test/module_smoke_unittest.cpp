// Smoke test for the scada.opcua_bridge module facade: the opcua:: type
// universe is third-party and not exported, so the conversion calls here
// include the opcuapp headers textually alongside the import - the
// documented pattern for bridge consumers.

#include <opcua/types/status.h>

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.opcua_bridge;

namespace scada_opcua_bridge_module {
namespace {

TEST(ScadaOpcuaBridgeModuleSmoke, StatusConversionRoundTrip) {
  // ToOpcua/ToScada overload sets from the import; opcua::StatusCode from
  // the textual include.
  opcua::StatusCode opcua_code =
      scada::opcua_bridge::ToOpcua(scada::StatusCode::Good);
  scada::StatusCode scada_code = scada::opcua_bridge::ToScada(opcua_code);
  EXPECT_EQ(scada_code, scada::StatusCode::Good);
}

TEST(ScadaOpcuaBridgeModuleSmoke, TransitiveSurfaces) {
  EXPECT_TRUE(scada::Status{scada::StatusCode::Good});
  EXPECT_EQ(Format(6), "6");
  scada::base::Check(true, "opcua_bridge module smoke");
}

}  // namespace
}  // namespace scada_opcua_bridge_module
