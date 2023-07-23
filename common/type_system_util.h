#pragma once

#include "common/type_system.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

inline bool WantsBrowseDirection(scada::BrowseDirection direction,
                                 bool forward) {
  if (direction == scada::BrowseDirection::Both)
    return true;

  const bool wants_forward = direction == scada::BrowseDirection::Forward;
  return forward == wants_forward;
}

inline bool WantsReference(const TypeSystem& type_system,
                           const scada::BrowseDescription& description,
                           const scada::NodeId& reference_type_id,
                           bool forward) {
  if (!WantsBrowseDirection(description.direction, forward))
    return false;

  if (description.include_subtypes) {
    return type_system.IsSubtypeOf(reference_type_id,
                                   description.reference_type_id);
  } else {
    return reference_type_id == description.reference_type_id;
  }
}

inline bool WantsReferenceOfSupertype(
    const TypeSystem& type_system,
    const scada::BrowseDescription& description,
    const scada::NodeId& supertype_reference_type_id,
    bool forward) {
  if (!WantsBrowseDirection(description.direction, forward))
    return false;

  return type_system.IsSubtypeOf(description.reference_type_id,
                                 supertype_reference_type_id);
}

inline bool WantsTypeDefinition(const TypeSystem& type_system,
                                const scada::BrowseDescription& description) {
  return WantsReference(type_system, description, scada::id::HasTypeDefinition,
                        true);
}

inline bool WantsOrganizes(const TypeSystem& type_system,
                           const scada::BrowseDescription& description) {
  return WantsReference(type_system, description, scada::id::Organizes, true);
}

inline bool WantsParent(const TypeSystem& type_system,
                        const scada::BrowseDescription& description) {
  return WantsReference(type_system, description,
                        scada::id::HierarchicalReferences, false);
}

inline bool MightWantReferenceSubtype(
    const TypeSystem& type_system,
    const scada::BrowseDescription& description,
    const scada::NodeId& reference_type_id,
    bool forward) {
  if (description.direction != scada::BrowseDirection::Both) {
    const bool wants_forward =
        description.direction == scada::BrowseDirection::Forward;
    if (forward != wants_forward)
      return false;
  }

  if (description.include_subtypes) {
    return type_system.IsSubtypeOf(description.reference_type_id,
                                   reference_type_id);
  } else {
    return reference_type_id == description.reference_type_id;
  }
}
