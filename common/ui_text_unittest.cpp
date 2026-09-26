#include "common/format.h"
#include "scada/variant.h"

#include <gmock/gmock.h>

namespace scada {
namespace {

// Answers with a recognizable stand-in, so the assertions say plainly which
// label was asked for without depending on a real catalog.
std::u16string RecordingFallbackLabel(FallbackLabel label) {
  switch (label) {
    case FallbackLabel::kDefaultClose:
      return u"<close>";
    case FallbackLabel::kDefaultOpen:
      return u"<open>";
    case FallbackLabel::kEmptyDisplayName:
      return u"<empty>";
    case FallbackLabel::kUnknownDisplayName:
      return u"<unknown>";
  }
  return {};
}

class UiTextTest : public ::testing::Test {
 protected:
  void TearDown() override {
    SetFallbackLabelProvider(nullptr);
    SetBooleanTextProvider(nullptr);
  }
};

// Without a provider — the server, and unit tests — the invariant forms
// render. This library carries no operator-facing words for them.
TEST_F(UiTextTest, WithoutAProviderTheInvariantFormsRender) {
  EXPECT_EQ(DefaultCloseLabel(), u"1");
  EXPECT_EQ(DefaultOpenLabel(), u"0");
  EXPECT_EQ(EmptyDisplayName(), u"#NAME?");
  EXPECT_EQ(UnknownDisplayName(), u"#NAME?");
}

// The labels are looked up per call, so a provider installed after startup
// (or a locale switch behind it) is picked up instead of being frozen into a
// constant at load time.
TEST_F(UiTextTest, FallbackLabelsGoThroughTheProvider) {
  SetFallbackLabelProvider(&RecordingFallbackLabel);

  EXPECT_EQ(DefaultCloseLabel(), u"<close>");
  EXPECT_EQ(DefaultOpenLabel(), u"<open>");
  EXPECT_EQ(EmptyDisplayName(), u"<empty>");
  EXPECT_EQ(UnknownDisplayName(), u"<unknown>");
}

// FormatTs falls back to the default labels only when the item's TsFormat
// carries none; an explicit label wins and does not go through the provider
// (it is configuration data, already in the operator's language).
TEST_F(UiTextTest, FormatTsUsesTheProviderOnlyForTheFallbackLabels) {
  SetFallbackLabelProvider(&RecordingFallbackLabel);

  EXPECT_EQ(FormatTs(true).text, u"<close>");
  EXPECT_EQ(FormatTs(false).text, u"<open>");

  const TsFormatParams params{.close_label = LocalizedText{u"Closed"},
                              .open_label = LocalizedText{u"Opened"}};
  EXPECT_EQ(FormatTs(true, params).text, u"Closed");
  EXPECT_EQ(FormatTs(false, params).text, u"Opened");
}

// Stands in for the client's boolean words, so the round trip below is
// exercised through a provider without depending on a real catalog.
std::u16string RecordingBooleanText(bool value) {
  return value ? u"<yes>" : u"<no>";
}

// A BOOL exported by one side must import on the other. Which words land in the
// file depends on whether the writer had a provider installed — a server has
// none and writes the invariant `true`/`false` — and older files carry the
// English or the Russian unconditionally, so the parse has to accept all of
// them, in either direction.
TEST_F(UiTextTest, BoolLabelsParseBackWhicheverLocaleWroteThem) {
  const auto parse = [](std::u16string_view text) {
    scada::Variant value;
    EXPECT_TRUE(StringToValue(text, scada::Variant::Type::BOOL, value))
        << "failed to parse a BOOL label";
    return value.get_or(false);
  };

  SetBooleanTextProvider(nullptr);
  EXPECT_EQ(scada::Variant::TrueLabel(), u"true");
  EXPECT_EQ(scada::Variant::FalseLabel(), u"false");
  EXPECT_TRUE(parse(scada::Variant::TrueLabel()));
  EXPECT_FALSE(parse(scada::Variant::FalseLabel()));
  EXPECT_TRUE(parse(u"Yes"));
  EXPECT_FALSE(parse(u"No"));
  EXPECT_TRUE(parse(u"Да"));
  EXPECT_FALSE(parse(u"Нет"));

  SetBooleanTextProvider(&RecordingBooleanText);
  EXPECT_TRUE(parse(scada::Variant::TrueLabel()));
  EXPECT_FALSE(parse(scada::Variant::FalseLabel()));
  // Still every spelling another writer may have used.
  EXPECT_TRUE(parse(u"true"));
  EXPECT_FALSE(parse(u"false"));
  EXPECT_TRUE(parse(u"Yes"));
  EXPECT_FALSE(parse(u"No"));
  EXPECT_TRUE(parse(u"Да"));
  EXPECT_FALSE(parse(u"Нет"));
  SetBooleanTextProvider(nullptr);
}

}  // namespace
}  // namespace scada
