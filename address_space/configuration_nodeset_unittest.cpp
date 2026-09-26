#include "address_space/configuration_nodeset.h"

#include "scada/locale_negotiation.h"

#include <gmock/gmock.h>

#include <chrono>
#include <cmath>

using namespace std::chrono_literals;
using namespace testing;

namespace scada {
namespace {

// In-process NamespaceArray for these tests: 1 = urn:a, 2 = urn:b, 3 = urn:c.
const std::vector<std::string> kUris{"http://opcfoundation.org/UA/", "urn:a",
                                     "urn:b", "urn:c"};

// Properties of the one type these tests use, both directions.
const NodeId kType{NumericId{100}, 3};
const NodeId kAliasDecl{NumericId{140}, 3};
const NodeId kSeverityDecl{NumericId{144}, 3};
const NodeId kSourceDecl{NumericId{150}, 3};
const NodeId kLabelDecl{NumericId{151}, 3};
const NodeId kSinceDecl{NumericId{152}, 3};
const NodeId kBlobDecl{NumericId{153}, 3};
const NodeId kEnabledDecl{NumericId{154}, 3};
const NodeId kGainDecl{NumericId{155}, 3};

const std::vector<std::pair<NodeId, std::string>> kProperties{
    {kAliasDecl, "Alias"},     {kSeverityDecl, "Severity"},
    {kSourceDecl, "Source"},   {kLabelDecl, "Label"},
    {kSinceDecl, "Since"},     {kBlobDecl, "Blob"},
    {kEnabledDecl, "Enabled"}, {kGainDecl, "Gain"}};

NodeSetPropertyNames Names() {
  return {
      .browse_name = [](const NodeId& type,
                        const NodeId& decl) -> std::optional<QualifiedName> {
        if (type != kType) {
          return std::nullopt;
        }
        for (const auto& [id, name] : kProperties) {
          if (id == decl) {
            return QualifiedName{name, 3};
          }
        }
        return std::nullopt;
      },
      .declaration = [](const NodeId& type,
                        const QualifiedName& name) -> NodeId {
        if (type != kType) {
          return {};
        }
        for (const auto& [id, property_name] : kProperties) {
          if (property_name == name.name() && name.namespace_index() == 3) {
            return id;
          }
        }
        return {};
      },
  };
}

Time At(int seconds_after_epoch_2026) {
  using namespace std::chrono;
  return time_point_cast<Time::duration>(sys_days{2026y / September / 26} +
                                         seconds{seconds_after_epoch_2026});
}

// A node exercising every part of the format: two namespaces, a parent, a
// type, a non-hierarchical reference, a two-language DisplayName, and one
// property of every value type configuration uses.
NodeState MakeNode() {
  const std::vector<LocalizedText> names{{"ru", u"Насос"}, {"en", u"Pump"}};
  return NodeState{
      .node_id = NodeId{NumericId{7}, 1},
      .node_class = NodeClass::Object,
      .type_definition_id = kType,
      .parent_id = NodeId{NumericId{1}, 2},
      .reference_type_id = NodeId{NumericId{35}},
      .attributes = {.browse_name = QualifiedName{"Pump", 1},
                     .display_name = EncodeMultiLanguage(names)},
      .properties = {{kAliasDecl, Variant{String{"ts106"}}},
                     {kSeverityDecl, Variant{Int32{90}}},
                     {kSourceDecl, Variant{NodeId{NumericId{12}, 2}}},
                     {kLabelDecl, Variant{LocalizedText{"en", u"Main, \"A\""}}},
                     {kSinceDecl, Variant{At(3723) + Duration{123456}}},
                     {kBlobDecl, Variant{ByteString{'\0', '\x7f', '\xff'}}},
                     {kEnabledDecl, Variant{true}},
                     {kGainDecl, Variant{0.1}}},
      .references = {{.reference_type_id = NodeId{NumericId{9}, 3},
                      .forward = true,
                      .node_id = NodeId{NumericId{2}, 2}}}};
}

ConfigurationNodeSet MakeNodeSet() {
  return {.nodes = {MakeNode()},
          .version = "sha256:abc",
          .scope = {"urn:a", "urn:b"},
          .last_modified = At(0)};
}

void ExpectSameNode(const NodeState& actual, const NodeState& expected) {
  EXPECT_EQ(actual.node_id, expected.node_id);
  EXPECT_EQ(actual.node_class, expected.node_class);
  EXPECT_EQ(actual.type_definition_id, expected.type_definition_id);
  EXPECT_EQ(actual.parent_id, expected.parent_id);
  EXPECT_EQ(actual.reference_type_id, expected.reference_type_id);
  EXPECT_EQ(actual.attributes.browse_name, expected.attributes.browse_name);
  EXPECT_EQ(DecodeMultiLanguage(actual.attributes.display_name),
            DecodeMultiLanguage(expected.attributes.display_name));
  EXPECT_EQ(actual.properties, expected.properties);
  EXPECT_EQ(actual.references, expected.references);
}

// The format's contract: what the writer writes, the reader reads back as the
// same configuration — every field, every value type, both languages.
TEST(ConfigurationNodeSetTest, RoundTripsEveryPartOfANode) {
  const ConfigurationNodeSet nodeset = MakeNodeSet();

  const auto xml = WriteConfigurationNodeSet(nodeset, kUris, Names());
  ASSERT_TRUE(xml.ok()) << xml.status();
  const auto read = ReadConfigurationNodeSet(*xml, kUris, Names());
  ASSERT_TRUE(read.ok()) << read.status();

  ASSERT_EQ(read->nodes.size(), 1u);
  ExpectSameNode(read->nodes[0], nodeset.nodes[0]);
  EXPECT_EQ(read->version, "sha256:abc");
  EXPECT_THAT(read->scope, ElementsAre("urn:a", "urn:b"));
  EXPECT_EQ(read->last_modified, At(0));
}

// Namespaces travel by URI (Part 6 §F.2): a server whose NamespaceArray puts
// the same URIs at other indexes reads the same nodes.
TEST(ConfigurationNodeSetTest, NamespacesTravelByUriNotByIndex) {
  const auto xml = WriteConfigurationNodeSet(MakeNodeSet(), kUris, Names());
  ASSERT_TRUE(xml.ok());

  // urn:c at 1, urn:a at 2, urn:b at 3 — everything shifted.
  const std::vector<std::string> other{"http://opcfoundation.org/UA/", "urn:c",
                                       "urn:a", "urn:b"};
  NodeSetPropertyNames names = Names();
  names.declaration = [](const NodeId& type, const QualifiedName& name) {
    // kType and its declarations now live at index 1 (urn:c).
    return type == NodeId{NumericId{100}, 1} && name.name() == "Alias"
               ? NodeId{NumericId{140}, 1}
               : NodeId{NumericId{999}, 1};
  };
  const auto read = ReadConfigurationNodeSet(*xml, other, names);
  ASSERT_TRUE(read.ok()) << read.status();

  const NodeState& node = read->nodes.at(0);
  EXPECT_EQ(node.node_id, (NodeId{NumericId{7}, 2}));
  EXPECT_EQ(node.parent_id, (NodeId{NumericId{1}, 3}));
  EXPECT_EQ(node.type_definition_id, (NodeId{NumericId{100}, 1}));
  EXPECT_EQ(node.attributes.browse_name, (QualifiedName{"Pump", 2}));
  EXPECT_THAT(node.properties,
              Contains(Pair(NodeId{NumericId{999}, 1},
                            Variant{NodeId{NumericId{12}, 3}})));
}

// Equal configuration gives byte-equal documents whatever order the nodes
// came in — what lets a hash of the document identify a configuration.
TEST(ConfigurationNodeSetTest, OutputDoesNotDependOnNodeOrder) {
  ConfigurationNodeSet forward = MakeNodeSet();
  NodeState second = MakeNode();
  second.node_id = NodeId{NumericId{3}, 1};
  forward.nodes.push_back(second);
  ConfigurationNodeSet backward = forward;
  std::ranges::reverse(backward.nodes);

  EXPECT_EQ(WriteConfigurationNodeSet(forward, kUris, Names()),
            WriteConfigurationNodeSet(backward, kUris, Names()));
}

// The document is a standard UANodeSet, with properties as PropertyType
// children under HasProperty named by their BrowseName.
TEST(ConfigurationNodeSetTest, WritesAStandardUANodeSet) {
  const auto xml = WriteConfigurationNodeSet(MakeNodeSet(), kUris, Names());
  ASSERT_TRUE(xml.ok());

  EXPECT_THAT(*xml, HasSubstr("<UANodeSet xmlns=\"http://opcfoundation.org/UA/"
                              "2011/03/UANodeSet.xsd\""));
  EXPECT_THAT(*xml, HasSubstr("<Uri>urn:a</Uri>"));
  EXPECT_THAT(*xml,
              HasSubstr("<UAObject NodeId=\"ns=1;i=7\" "
                        "BrowseName=\"1:Pump\" ParentNodeId=\"ns=2;i=1\""));
  EXPECT_THAT(*xml,
              HasSubstr("<DisplayName Locale=\"ru\">Насос</DisplayName>"));
  EXPECT_THAT(*xml,
              HasSubstr("<UAVariable NodeId=\"ns=1;s=7!Alias\" "
                        "BrowseName=\"3:Alias\" ParentNodeId=\"ns=1;i=7\""));
  EXPECT_THAT(*xml,
              HasSubstr("<Reference ReferenceType=\"i=40\">i=68</Reference>"));
  EXPECT_THAT(*xml, HasSubstr("<uax:String>ts106</uax:String>"));
  EXPECT_THAT(*xml, HasSubstr("<uax:DateTime>2026-09-26T01:02:03.1234560Z"));
}

TEST(ConfigurationNodeSetTest, SpecialDoublesUseTheXmlSpellings) {
  ConfigurationNodeSet nodeset = MakeNodeSet();
  nodeset.nodes[0].properties = {{kGainDecl, Variant{HUGE_VAL}}};

  const auto xml = WriteConfigurationNodeSet(nodeset, kUris, Names());
  ASSERT_TRUE(xml.ok());
  EXPECT_THAT(*xml, HasSubstr("<uax:Double>INF</uax:Double>"));
  const auto read = ReadConfigurationNodeSet(*xml, kUris, Names());
  ASSERT_TRUE(read.ok());
  // Compared as a double: Variant's equality takes a difference, and
  // infinity minus infinity is NaN, so it never equals itself.
  ASSERT_EQ(read->nodes[0].properties.size(), 1u);
  const double gain = read->nodes[0].properties[0].second.get<double>();
  EXPECT_TRUE(std::isinf(gain) && gain > 0);
}

TEST(ConfigurationNodeSetTest, AnUnnamedPropertyIsNotWritten) {
  ConfigurationNodeSet nodeset = MakeNodeSet();
  nodeset.nodes[0].properties.emplace_back(NodeId{NumericId{999}, 3},
                                           Variant{Int32{1}});

  EXPECT_EQ(WriteConfigurationNodeSet(nodeset, kUris, Names()).status().code(),
            StatusCode::Bad_WrongPropertyId);
}

TEST(ConfigurationNodeSetTest, UnrepresentableContentIsRefused) {
  ConfigurationNodeSet arrays = MakeNodeSet();
  arrays.nodes[0].properties = {
      {kSeverityDecl, Variant{std::vector<Int32>{1, 2}}}};
  EXPECT_EQ(WriteConfigurationNodeSet(arrays, kUris, Names()).status().code(),
            StatusCode::Bad_NotSupported);

  ConfigurationNodeSet method = MakeNodeSet();
  method.nodes[0].node_class = NodeClass::Method;
  EXPECT_EQ(WriteConfigurationNodeSet(method, kUris, Names()).status().code(),
            StatusCode::Bad_NotSupported);

  ConfigurationNodeSet unknown_namespace = MakeNodeSet();
  unknown_namespace.nodes[0].node_id = NodeId{NumericId{7}, 9};
  EXPECT_EQ(WriteConfigurationNodeSet(unknown_namespace, kUris, Names())
                .status()
                .code(),
            StatusCode::Bad_WrongNodeId);
}

// A file naming a namespace this server does not have cannot be placed.
TEST(ConfigurationNodeSetTest, AnUnknownNamespaceUriIsRefusedOnRead) {
  const auto xml = WriteConfigurationNodeSet(MakeNodeSet(), kUris, Names());
  ASSERT_TRUE(xml.ok());
  const std::vector<std::string> missing_b{"http://opcfoundation.org/UA/",
                                           "urn:a", "urn:c"};

  EXPECT_EQ(ReadConfigurationNodeSet(*xml, missing_b, Names()).status().code(),
            StatusCode::Bad_WrongNodeId);
}

TEST(ConfigurationNodeSetTest, AnUnresolvablePropertyIsRefusedOnRead) {
  const auto xml = WriteConfigurationNodeSet(MakeNodeSet(), kUris, Names());
  ASSERT_TRUE(xml.ok());
  NodeSetPropertyNames names = Names();
  names.declaration = [](const NodeId&, const QualifiedName&) {
    return NodeId{};
  };

  EXPECT_EQ(ReadConfigurationNodeSet(*xml, kUris, names).status().code(),
            StatusCode::Bad_WrongPropertyId);
}

TEST(ConfigurationNodeSetTest, MalformedDocumentsAreRefused) {
  EXPECT_EQ(
      ReadConfigurationNodeSet("<UANodeSet", kUris, Names()).status().code(),
      StatusCode::Bad_CantParseString);
  EXPECT_EQ(
      ReadConfigurationNodeSet("<Other/>", kUris, Names()).status().code(),
      StatusCode::Bad_CantParseString);
}

}  // namespace
}  // namespace scada
