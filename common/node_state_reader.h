#pragma once

#include "base/common_types.h"
#include "common/node_state.h"
#include "core/variant_utils.h"

#include <chrono>

template <class T>
class NodeStateReader {
 public:
  explicit NodeStateReader(const scada::NodeState& node_state)
      : node_state_{node_state} {}

  template <class F>
  NodeStateReader& property(const scada::NodeId& prop_decl_id, F T::*field) {
    auto i = std::find_if(
        node_state_.properties.begin(), node_state_.properties.end(),
        [&](const auto& p) { return p.first == prop_decl_id; });
    if (i == node_state_.properties.end())
      return *this;

    const auto& property_value = i->second;
    if (property_value.is_null())
      return *this;

    if (!ConvertVariant(property_value, result_.*field)) {
      assert(false);
      ok_ = false;
      return *this;
    }

    return *this;
  }

  std::optional<T> Read() {
    return ok_ ? std::optional<T>{std::move(result_)} : std::optional<T>{};
  }

 private:
  const scada::NodeState& node_state_;

  bool ok_ = true;
  T result_;
};

class NodeStateReader2 {
 public:
  explicit NodeStateReader2(const scada::NodeState& node_state)
      : node_state_{node_state} {}

  template <class T>
  NodeStateReader2& ReadProperty(const scada::NodeId& prop_decl_id, T& value) {
    const auto* found_value =
        scada::FindProperty(node_state_.properties, prop_decl_id);
    if (!found_value)
      return *this;

    if (!ConvertVariant(*found_value, value)) {
      assert(false);
      ok_ = false;
      return *this;
    }

    return *this;
  }

  NodeStateReader2& ReadReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  scada::NodeId& target_id) {
    const auto* found_target_id = scada::FindReference(
        node_state_.references, reference_type_id, forward);
    if (found_target_id)
      target_id = *found_target_id;

    return *this;
  }

  template <class E>
  NodeStateReader2& ReadEnum(const scada::NodeId& prop_decl_id, E& value) {
    // Use a separate implementation to avoid overriding current |duration|
    // value if property isn't set.

    const auto* found_value =
        scada::FindProperty(node_state_.properties, prop_decl_id);
    if (!found_value)
      return *this;

    scada::UInt32 int_value = 0;
    if (!ConvertVariant(*found_value, int_value)) {
      assert(false);
      ok_ = false;
      return *this;
    }

    value = static_cast<E>(int_value);
    return *this;
  }

  template <class T>
  NodeStateReader2& ReadDuration(const scada::NodeId& prop_decl_id,
                                 Duration& duration) {
    // Use a separate implementation to avoid overriding current |duration|
    // value if property isn't set.

    const auto* found_value =
        scada::FindProperty(node_state_.properties, prop_decl_id);
    if (!found_value)
      return *this;

    scada::UInt32 duration_value = 0;
    if (!ConvertVariant(*found_value, duration_value)) {
      assert(false);
      ok_ = false;
      return *this;
    }

    duration = T{duration_value};
    return *this;
  }

 private:
  const scada::NodeState& node_state_;
  bool ok_ = true;
};
