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

  const scada::NodeState& node_state_;

 private:
  bool ok_ = true;
  T result_;
};
