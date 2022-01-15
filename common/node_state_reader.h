#pragma once

#include "common/node_state.h"
#include "core/variant_utils.h"

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
    const auto* found_target_id =
        scada::FindReference(node_state_.references, reference_type_id, true);
    if (!found_target_id)
      return *this;

    target_id = *found_target_id;
    return *this;
  }

 private:
  const scada::NodeState& node_state_;
  bool ok_ = true;
};
