#pragma once

#include "core/attribute_ids.h"
#include "core/debug_util.h"
#include "core/localized_text.h"
#include "core/node_id.h"
#include "core/qualified_name.h"
#include "core/variant.h"

namespace scada {

class AttributeSet {
 public:
  void Add(AttributeId id) { bits_ |= 1 << static_cast<unsigned>(id); }
  void Remove(AttributeId id) { bits_ &= ~(1 << static_cast<unsigned>(id)); }

  bool has(AttributeId id) const {
    return (bits_ & (1 << static_cast<unsigned>(id))) != 0;
  }

  bool empty() const { return bits_ == 0; }

 private:
  unsigned bits_ = 0;
};

struct NodeAttributes {
  NodeAttributes& set_browse_name(QualifiedName browse_name) {
    this->browse_name = std::move(browse_name);
    return *this;
  }
  NodeAttributes& set_display_name(LocalizedText display_name) {
    this->display_name = std::move(display_name);
    return *this;
  }
  NodeAttributes& set_data_type(NodeId data_type) {
    this->data_type = std::move(data_type);
    return *this;
  }
  NodeAttributes& set_value(Variant value) {
    this->value = std::move(value);
    return *this;
  }

  bool empty() const {
    return browse_name.empty() && display_name.empty() && data_type.is_null() &&
           value.has_value();
  }

  QualifiedName browse_name;
  LocalizedText display_name;
  NodeId data_type;
  std::optional<Variant> value;
};

}  // namespace scada

inline std::ostream& operator<<(std::ostream& stream,
                                const scada::NodeAttributes& attributes) {
  return stream << "{"
                << "browse_name: " << attributes.browse_name << ", "
                << "display_name: " << attributes.display_name << ", "
                << "data_type: " << attributes.data_type << ", "
                << "value: " << attributes.value << "}";
}
