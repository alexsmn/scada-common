// Smoke test for the scada.timed_data module facade: names come from
// `import scada.timed_data;` only, including the surfaces re-exported
// through `export import scada.common` / `export import scada.events`.

#include <vector>

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.timed_data;

namespace scada_timed_data_module {
namespace {

TEST(ScadaTimedDataModuleSmoke, FakeTimedDataAndView) {
  FakeTimedData timed_data;
  timed_data.title = u"smoke";
  EXPECT_EQ(timed_data.title, u"smoke");

  TimedDataView view;
  scada::DataValue data_value;
  data_value.source_timestamp = scada::DateTime::Now();
  view.InsertOrUpdate(data_value);
  EXPECT_EQ(view.values().size(), 1u);

  // Extern constants defined out-of-line in the library (proves linkage
  // through the module boundary; the "current only" ready-range list is
  // empty by definition).
  std::vector<scada::DateTimeRange> ready_ranges = kReadyCurrentTimeOnly;
  EXPECT_TRUE(ready_ranges.empty());
}

TEST(ScadaTimedDataModuleSmoke, PropertySetAndTransitiveSurfaces) {
  PropertySet properties{PROPERTY_TITLE};
  EXPECT_TRUE(properties.is_title_changed());
  EXPECT_FALSE(properties.is_current_changed());

  // Via export import chain: scada.events / scada.common / scada.core.
  EventStorage storage;
  storage.Clear();
  EXPECT_TRUE(scada::Status{scada::StatusCode::Good});
  EXPECT_EQ(Format(9), "9");
}

}  // namespace
}  // namespace scada_timed_data_module
