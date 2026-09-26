#include "address_space/uanodeset_xml.h"

#include <gmock/gmock.h>

#include <chrono>

using namespace std::chrono_literals;
using namespace testing;

namespace scada::uanodeset {
namespace {

Time At(std::chrono::sys_days day, Duration since_midnight = {}) {
  return std::chrono::time_point_cast<Duration>(day) + since_midnight;
}

// Reads the value of `<Value>` + `inner` + `</Value>`.
std::optional<Variant> Read(std::string_view inner) {
  const std::string xml = std::format(
      "<Value xmlns:uax=\"http://opcfoundation.org/UA/2008/02/Types.xsd\">{}"
      "</Value>",
      inner);
  pugi::xml_document document;
  EXPECT_TRUE(document.load_buffer(xml.data(), xml.size()));
  const std::vector<std::string> uris;
  const NamespaceTable namespaces{uris};
  return ReadValue(document.child("Value"), namespaces, {});
}

// Part 6 §5.3.1.6: the earliest DateTime is the null one and "shall be
// encoded in XML as '0001-01-01T00:00:00Z'", the latest as
// '9999-12-31T23:59:59Z' — never literally.
TEST(UaNodeSetXmlTest, TheNullAndLatestTimesUseTheirWrittenForms) {
  EXPECT_EQ(TimeText(kNullTime), "0001-01-01T00:00:00Z");
  EXPECT_EQ(TimeText(kMinTime), "0001-01-01T00:00:00Z");
  EXPECT_EQ(TimeText(kMaxTime), "9999-12-31T23:59:59Z");

  EXPECT_EQ(ParseTimeText("0001-01-01T00:00:00Z"), kNullTime);
  EXPECT_EQ(ParseTimeText("9999-12-31T23:59:59Z"), kMaxTime);
  // A file written before these rules spelled null as the epoch itself.
  EXPECT_EQ(ParseTimeText("1601-01-01T00:00:00Z"), kNullTime);
}

TEST(UaNodeSetXmlTest, OrdinaryTimesRoundTrip) {
  using namespace std::chrono;
  const Time time = At(sys_days{2026y / September / 26}, 1h + 2min + 3s + 4us);

  EXPECT_EQ(TimeText(time), "2026-09-26T01:02:03.0000040Z");
  EXPECT_EQ(ParseTimeText(TimeText(time)), time);
}

// §5.3.1.6 allows an explicit zone as well as Z, and lists a zoneless value
// as incorrect.
TEST(UaNodeSetXmlTest, ExplicitZonesAreAppliedAndZonelessTimesRefused) {
  using namespace std::chrono;
  EXPECT_EQ(ParseTimeText("2002-10-10T00:00:00+05:00"),
            At(sys_days{2002y / October / 9}, 19h));
  EXPECT_EQ(ParseTimeText("2002-10-09T19:00:00Z"),
            At(sys_days{2002y / October / 9}, 19h));
  EXPECT_EQ(ParseTimeText("2002-10-09T19:00:00"), std::nullopt);
}

TEST(UaNodeSetXmlTest, ReadsEveryScalarTheModelUses) {
  EXPECT_EQ(Read("<uax:Boolean>true</uax:Boolean>"), Variant{true});
  EXPECT_EQ(Read("<uax:Int32>-5</uax:Int32>"), Variant{Int32{-5}});
  EXPECT_EQ(Read("<uax:UInt32>7</uax:UInt32>"), Variant{UInt32{7}});
  EXPECT_EQ(Read("<uax:Double>0.5</uax:Double>"), Variant{0.5});
  EXPECT_EQ(Read("<uax:String>x</uax:String>"), Variant{String{"x"}});
  EXPECT_EQ(Read("<uax:ByteString></uax:ByteString>"), Variant{ByteString{}});
  EXPECT_EQ(Read("<uax:DateTime>0001-01-01T00:00:00Z</uax:DateTime>"),
            Variant{kNullTime});
  // The repository's own format wrote a DateTime as a bare integer; the
  // standard encoding does not, and neither does a model file any more.
  EXPECT_EQ(Read("<uax:DateTime>0</uax:DateTime>"), std::nullopt);
}

// The static model loader resolves every vendor URI to one namespace; the
// configuration codec resolves by an exact table and refuses the unknown.
TEST(UaNodeSetXmlTest, AnUnknownUriResolvesToTheFallbackOrFails) {
  const std::vector<std::string> uris{"http://opcfoundation.org/UA/", "urn:a"};
  NamespaceTable exact{uris};
  EXPECT_TRUE(exact.AddLocal("urn:a"));
  EXPECT_FALSE(exact.AddLocal("urn:other"));

  NamespaceTable with_fallback{{}, NamespaceIndex{7}};
  EXPECT_TRUE(with_fallback.AddLocal("urn:other"));
  EXPECT_EQ(with_fallback.FromLocal(1), NamespaceIndex{7});
  EXPECT_EQ(ParseNodeIdText("ns=1;i=42", with_fallback, {}),
            (NodeId{NumericId{42}, 7}));
  // A document index the document never declared resolves to nothing.
  EXPECT_EQ(ParseNodeIdText("ns=2;i=42", with_fallback, {}), std::nullopt);
}

// An alias the document does not declare is an error, not a null id: the
// static loader used to read one as null and drop the DataType it named.
TEST(UaNodeSetXmlTest, AnUndeclaredAliasDoesNotParse) {
  const std::vector<std::string> uris;
  const NamespaceTable namespaces{uris};
  EXPECT_EQ(ParseNodeIdText("UInt32", namespaces, {}), std::nullopt);
  EXPECT_EQ(ParseNodeIdText("UInt32", namespaces, {{"UInt32", "i=7"}}),
            NodeId{NumericId{7}});
}

}  // namespace
}  // namespace scada::uanodeset
