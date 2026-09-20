// Locale-qualified DisplayName in the shipped nodesets: the static address
// space's half of OPC UA locale negotiation (Part 4 §5.4,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4).
//
// `DisplayName` is `maxOccurs="unbounded"` in UANodeSet.xsd and its type
// carries a `Locale` attribute, so a translated node is written as sibling
// elements and the loader packs them into the multi-language form the
// client-facing service boundary resolves per session.
//
// This runs against the REAL nodesets rather than a synthetic document: the
// packing rules themselves are pinned by core's locale_negotiation tests, and
// what can actually regress here is a nodeset edit that drops a translation or
// an author who writes one language where two are meant.

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_util.h"
#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "model/data_items_node_ids.h"
#include "model/static_nodesets.h"
#include "scada/locale_negotiation.h"

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

namespace scada {
namespace {

std::vector<NodeState> LoadStaticNodes() {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  EXPECT_TRUE(
      LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space, factory));
  return MakeNodeStates(space);
}

const NodeState* FindNode(const std::vector<NodeState>& nodes,
                          const NodeId& node_id) {
  for (const NodeState& node : nodes) {
    if (node.node_id == node_id)
      return &node;
  }
  return nullptr;
}

// The tree root an operator sees first, and the node that exposed this gap:
// it read «Все объекты» to an English session because a nodeset DisplayName
// cannot be reached by a configuration-database translation row.
TEST(AddressSpaceNodesetLocaleTest, TheDataItemsRootIsTranslated) {
  const std::vector<NodeState> nodes = LoadStaticNodes();
  const NodeState* root = FindNode(nodes, data_items::id::DataItems);
  ASSERT_NE(root, nullptr);

  const LocalizedText& name = root->attributes.display_name;
  EXPECT_EQ(String{"mul"}, name.locale);
  EXPECT_EQ((std::vector<LocalizedText>{{"ru", u"Все объекты"},
                                        {"en", u"All objects"}}),
            DecodeMultiLanguage(name));
}

TEST(AddressSpaceNodesetLocaleTest, EachSessionResolvesItsOwnLanguage) {
  const std::vector<NodeState> nodes = LoadStaticNodes();
  const NodeState* root = FindNode(nodes, data_items::id::DataItems);
  ASSERT_NE(root, nullptr);

  const std::vector<String> english{"en"};
  const std::vector<String> russian{"ru"};
  EXPECT_EQ(u"All objects",
            ResolveLocalizedText(root->attributes.display_name, english).text);
  EXPECT_EQ(u"Все объекты",
            ResolveLocalizedText(root->attributes.display_name, russian).text);
}

TEST(AddressSpaceNodesetLocaleTest, AnUnmatchedLanguageFallsBackToRussian) {
  // Part 4 §5.4: an unsatisfiable request still gets an available locale.
  // Document order decides which, and the nodesets are authored in Russian.
  const std::vector<NodeState> nodes = LoadStaticNodes();
  const NodeState* root = FindNode(nodes, data_items::id::DataItems);
  ASSERT_NE(root, nullptr);

  const std::vector<String> japanese{"ja"};
  EXPECT_EQ(u"Все объекты",
            ResolveLocalizedText(root->attributes.display_name, japanese).text);
}

// The sweep's own guard: no shipped nodeset node may still be Russian-only.
// A new node added with a bare Russian <DisplayName> fails here, which is the
// regression this whole change is worth protecting.
TEST(AddressSpaceNodesetLocaleTest, NoStaticNodeIsRussianOnly) {
  const std::vector<NodeState> nodes = LoadStaticNodes();

  const auto is_cyrillic = [](char16_t c) {
    return (c >= u'А' && c <= u'я') || c == u'Ё' ||
           c == u'ё';
  };

  std::set<std::string> offenders;
  for (const NodeState& node : nodes) {
    const std::vector<LocalizedText> translations =
        DecodeMultiLanguage(node.attributes.display_name);
    const bool has_cyrillic = std::ranges::any_of(
        translations, [&](const LocalizedText& text) {
          return std::ranges::any_of(text.text, is_cyrillic);
        });
    if (!has_cyrillic)
      continue;
    const bool has_english = std::ranges::any_of(
        translations,
        [](const LocalizedText& text) { return text.locale == "en"; });
    if (!has_english)
      offenders.insert(ToString(node.node_id));
  }

  // The four OPC UA standard folders (Root, Objects, Types, Server) still
  // live in opcua_base.xml, which is the repo-owned <AddressSpace> format
  // where displayName is an ATTRIBUTE — locale-qualified elements do not
  // apply to it. They translate when that file migrates to UANodeSet2; see
  // common/model/docs/uanodeset-migration.md and backlog 56b.
  // Exactly the four OPC UA standard folders (Root, Objects, Types, Server),
  // named rather than counted so a new Russian-only node is reported as
  // itself. They still live in opcua_base.xml, which is the repo-owned
  // <AddressSpace> format where displayName is an ATTRIBUTE, so
  // locale-qualified elements do not apply to them; they translate when that
  // file migrates to UANodeSet2 (common/model/docs/uanodeset-migration.md,
  // backlog 56b).
  EXPECT_EQ((std::set<std::string>{R"("i=84")", R"("i=85")", R"("i=86")",
                                   R"("i=2253")"}),
            offenders);
}

}  // namespace
}  // namespace scada
