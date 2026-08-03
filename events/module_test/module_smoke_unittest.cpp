// Smoke test for the scada.events module facade: names come from
// `import scada.events;` only, including the surfaces re-exported through
// `export import scada.common` (and transitively scada.core / scada.base).

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.events;

namespace scada_events_module {
namespace {

TEST(ScadaEventsModuleSmoke, EventStorageAndSet) {
  EventStorage storage;
  EXPECT_TRUE(storage.events().empty());
  EXPECT_EQ(storage.GetNodeEvents(scada::NodeId{42, 7}), nullptr);
  storage.Clear();

  EventSet event_set;
  EXPECT_TRUE(event_set.empty());
}

TEST(ScadaEventsModuleSmoke, TransitiveSurfaces) {
  // scada.common / scada.core / scada.base via export import chain.
  scada::NodeProperties properties;
  EXPECT_EQ(scada::FindProperty(properties, scada::NodeId{1, 0}), nullptr);
  EXPECT_EQ(Format(3), "3");
  scada::base::Check(true, "events module smoke");
}

}  // namespace
}  // namespace scada_events_module
