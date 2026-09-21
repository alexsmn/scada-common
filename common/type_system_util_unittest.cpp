#include "common/type_system_util.h"

#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

#include <gmock/gmock.h>

#include <map>

namespace {

// A TypeSystem over an explicit type -> supertype map. The real
// AddressSpaceTypeSystem needs a populated address space; the predicates under
// test only ever ask IsSubtypeOf, so the map answers the same question without
// one.
class MapTypeSystem : public TypeSystem {
 public:
  explicit MapTypeSystem(std::map<scada::NodeId, scada::NodeId> supertypes)
      : supertypes_{std::move(supertypes)} {}

  bool IsSubtypeOf(const scada::NodeId& type_definition_id,
                   const scada::NodeId& supertype_id) const override {
    for (scada::NodeId id = type_definition_id; !id.is_null();) {
      if (id == supertype_id)
        return true;
      auto i = supertypes_.find(id);
      if (i == supertypes_.end())
        return false;
      id = i->second;
    }
    return false;
  }

 private:
  const std::map<scada::NodeId, scada::NodeId> supertypes_;
};

MapTypeSystem MakeTypeSystem() {
  return MapTypeSystem{{
      {scada::id::HasComponent, scada::id::Aggregates},
      {scada::id::Aggregates, scada::id::HasChild},
      {scada::id::HasChild, scada::id::HierarchicalReferences},
      {scada::id::HierarchicalReferences, scada::id::References},
      {scada::id::HasTypeDefinition, scada::id::NonHierarchicalReferences},
      {scada::id::NonHierarchicalReferences, scada::id::References},
  }};
}

// A BrowseDescription with no reference_type_id set. This is also the
// default-constructed shape, which is what makes the defect below so quiet.
scada::BrowseDescription UnfilteredBrowse() {
  return scada::BrowseDescription{.direction = scada::BrowseDirection::Both};
}

// OPC UA Part 4 §5.9.2.2 Parameters: "If not specified then all References are
// returned and includeSubtypes is ignored."
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.9.2.2
//
// Before this rule was implemented the subtype walk looked for a null id and
// never found one, so an unfiltered browse matched no reference of any type.
TEST(TypeSystemUtilTest, AnUnspecifiedReferenceTypeWantsEveryReferenceType) {
  const MapTypeSystem type_system = MakeTypeSystem();
  const auto description = UnfilteredBrowse();

  EXPECT_TRUE(WantsReference(type_system, description, scada::id::HasComponent,
                             /*forward=*/true));
  EXPECT_TRUE(WantsReference(type_system, description,
                             scada::id::HasTypeDefinition, /*forward=*/true));
  // A type the map knows nothing about is still wanted: "all References"
  // cannot depend on the type being resolvable.
  EXPECT_TRUE(WantsReference(type_system, description, scada::NodeId{4242, 7},
                             /*forward=*/true));
}

// The same sentence says includeSubtypes is ignored, so the exact-id compare
// must not be reached either.
TEST(TypeSystemUtilTest, AnUnspecifiedReferenceTypeIgnoresIncludeSubtypes) {
  const MapTypeSystem type_system = MakeTypeSystem();
  auto description = UnfilteredBrowse();
  description.include_subtypes = false;

  EXPECT_TRUE(WantsReference(type_system, description, scada::id::HasComponent,
                             /*forward=*/true));
}

// "All References" is about the reference TYPE filter only. browseDirection is
// a separate parameter and still applies.
TEST(TypeSystemUtilTest, AnUnspecifiedReferenceTypeStillHonoursDirection) {
  const MapTypeSystem type_system = MakeTypeSystem();
  auto description = UnfilteredBrowse();
  description.direction = scada::BrowseDirection::Inverse;

  EXPECT_FALSE(WantsReference(type_system, description, scada::id::HasComponent,
                              /*forward=*/true));
  EXPECT_TRUE(WantsReference(type_system, description, scada::id::HasComponent,
                             /*forward=*/false));
}

// The null branch is a special case, not a relaxation: a named filter must
// still reject a reference outside its subtree.
TEST(TypeSystemUtilTest, ANamedReferenceTypeStillFilters) {
  const MapTypeSystem type_system = MakeTypeSystem();
  scada::BrowseDescription description{
      .direction = scada::BrowseDirection::Both,
      .reference_type_id = scada::id::HierarchicalReferences};

  EXPECT_TRUE(WantsReference(type_system, description, scada::id::HasComponent,
                             /*forward=*/true));
  EXPECT_FALSE(WantsReference(type_system, description,
                              scada::id::HasTypeDefinition, /*forward=*/true));
}

// WantsReferenceOfSupertype and MightWantReferenceSubtype ask the reverse
// question — "could anything under this supertype be wanted" — which a node
// manager uses to decide whether to produce a whole class of references at
// all. An unfiltered browse wants every class, so both must say yes.
TEST(TypeSystemUtilTest, AnUnspecifiedReferenceTypeWantsEverySupertype) {
  const MapTypeSystem type_system = MakeTypeSystem();
  const auto description = UnfilteredBrowse();

  EXPECT_TRUE(WantsReferenceOfSupertype(type_system, description,
                                        scada::id::HierarchicalReferences,
                                        /*forward=*/true));
  EXPECT_TRUE(WantsReferenceOfSupertype(type_system, description,
                                        scada::id::NonHierarchicalReferences,
                                        /*forward=*/true));
}

TEST(TypeSystemUtilTest, AnUnspecifiedReferenceTypeMightWantEverySubtype) {
  const MapTypeSystem type_system = MakeTypeSystem();
  const auto description = UnfilteredBrowse();

  EXPECT_TRUE(MightWantReferenceSubtype(type_system, description,
                                        scada::id::HierarchicalReferences,
                                        /*forward=*/true));
  EXPECT_TRUE(MightWantReferenceSubtype(type_system, description,
                                        scada::id::HasTypeDefinition,
                                        /*forward=*/true));
}

// The convenience wrappers route through WantsReference, so they inherit the
// rule; pinning them keeps a future refactor from reintroducing the strict
// compare in one of them alone.
TEST(TypeSystemUtilTest, TheNamedWrappersInheritTheRule) {
  const MapTypeSystem type_system = MakeTypeSystem();
  const auto description = UnfilteredBrowse();

  EXPECT_TRUE(WantsTypeDefinition(type_system, description));
  EXPECT_TRUE(WantsOrganizes(type_system, description));
  EXPECT_TRUE(WantsParent(type_system, description));
}

}  // namespace
