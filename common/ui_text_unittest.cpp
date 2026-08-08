#include "base/ui_text.h"

#include "common/format.h"

#include <gmock/gmock.h>

namespace scada {
namespace {

// Records what the formatting code asked to translate, and answers with a
// recognizable stand-in so the test does not depend on a real catalog.
std::u16string RecordingTranslator(std::string_view english) {
  return u"<" + std::u16string{english.begin(), english.end()} + u">";
}

class UiTextTest : public ::testing::Test {
 protected:
  void TearDown() override { SetUiTextTranslator(nullptr); }
};

TEST_F(UiTextTest, WithoutTranslatorReturnsTheEnglishSource) {
  SetUiTextTranslator(nullptr);

  EXPECT_EQ(TranslateUiText("On"), u"On");
  EXPECT_EQ(DefaultCloseLabel(), u"On");
  EXPECT_EQ(DefaultOpenLabel(), u"Off");
  EXPECT_EQ(EmptyDisplayName(), u"#NAME?");
  EXPECT_EQ(UnknownDisplayName(), u"#NAME?");
}

TEST_F(UiTextTest, InstalledTranslatorSuppliesTheDisplayText) {
  SetUiTextTranslator(&RecordingTranslator);

  EXPECT_EQ(TranslateUiText("On"), u"<On>");
}

// The point of the seam: the default labels are looked up per call, so a
// translator installed after startup (or a locale switch behind it) is picked
// up instead of being frozen into a constant at load time.
TEST_F(UiTextTest, DefaultLabelsGoThroughTheTranslator) {
  SetUiTextTranslator(&RecordingTranslator);

  EXPECT_EQ(DefaultCloseLabel(), u"<On>");
  EXPECT_EQ(DefaultOpenLabel(), u"<Off>");
  EXPECT_EQ(EmptyDisplayName(), u"<#NAME?>");
  EXPECT_EQ(UnknownDisplayName(), u"<#NAME?>");
}

// FormatTs falls back to the default labels only when the item's TsFormat
// carries none; an explicit label wins and is not translated (it is
// configuration data, already in the operator's language).
TEST_F(UiTextTest, FormatTsTranslatesOnlyTheFallbackLabels) {
  SetUiTextTranslator(&RecordingTranslator);

  EXPECT_EQ(FormatTs(true).text, u"<On>");
  EXPECT_EQ(FormatTs(false).text, u"<Off>");

  const TsFormatParams params{.close_label = LocalizedText{u"Closed"},
                              .open_label = LocalizedText{u"Opened"}};
  EXPECT_EQ(FormatTs(true, params).text, u"Closed");
  EXPECT_EQ(FormatTs(false, params).text, u"Opened");
}

// A BOOL exported by one side must import on the other. Which words land in the
// file depends on whether the writer had a catalog installed, and files written
// before the labels were translatable carry Russian unconditionally — so the
// parse has to accept all three, in either direction.
TEST_F(UiTextTest, BoolLabelsParseBackWhicheverLocaleWroteThem) {
  const auto parse = [](std::u16string_view text) {
    scada::Variant value;
    EXPECT_TRUE(StringToValue(text, scada::Variant::Type::BOOL, value))
        << "failed to parse a BOOL label";
    return value.get_or(false);
  };

  SetUiTextTranslator(nullptr);
  EXPECT_TRUE(parse(scada::Variant::TrueLabel()));
  EXPECT_FALSE(parse(scada::Variant::FalseLabel()));
  EXPECT_TRUE(parse(u"Да"));
  EXPECT_FALSE(parse(u"Нет"));

  SetUiTextTranslator(&RecordingTranslator);
  EXPECT_TRUE(parse(scada::Variant::TrueLabel()));
  EXPECT_FALSE(parse(scada::Variant::FalseLabel()));
  // Still the English and the legacy Russian, even with a catalog installed.
  EXPECT_TRUE(parse(u"Yes"));
  EXPECT_FALSE(parse(u"No"));
  EXPECT_TRUE(parse(u"Да"));
  EXPECT_FALSE(parse(u"Нет"));
}

}  // namespace
}  // namespace scada
