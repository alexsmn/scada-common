#pragma once

#include "common/type_system.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

inline bool WantsReference(const TypeSystem& type_system,
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
    return type_system.IsSubtypeOf(reference_type_id,
                                   description.reference_type_id);
  } else {
    return reference_type_id == description.reference_type_id;
  }
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
