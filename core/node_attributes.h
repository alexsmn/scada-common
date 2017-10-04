#pragma once

namespace scada {

class AttributeSet {
 public:
  void Add(AttributeId id) { bits_ |= 1 << static_cast<unsigned>(id); }
  void Remove(AttributeId id) { bits_ &= ~(1 << static_cast<unsigned>(id)); }

  bool has(AttributeId id) const { return (bits_ & (1 << static_cast<unsigned>(id))) != 0; }

  bool empty() const { return bits_ == 0; }

 private:
  unsigned bits_ = 0;
};

class NodeAttributes : public AttributeSet  {
 public:
  QualifiedName& browse_name() { return browse_name_; }
  const QualifiedName& browse_name() const { return browse_name_; }
  NodeAttributes& set_browse_name(QualifiedName value) {
    browse_name_ = std::move(value);
    Add(AttributeId::BrowseName);
    return *this;
  }

  LocalizedText& display_name() { return display_name_; }
  const LocalizedText& display_name() const { return display_name_; }
  NodeAttributes& set_display_name(LocalizedText value) {
    display_name_ = std::move(value);
    Add(AttributeId::DisplayName);
    return *this;
  }

  const NodeId& data_type_id() const { return data_type_id_; }
  NodeAttributes& set_data_type_id(NodeId data_type_id) {
    data_type_id_ = std::move(data_type_id);
    Add(AttributeId::DataType);
    return *this;
  }

  Variant& value() { return value_; }
  const Variant& value() const { return value_; }
  NodeAttributes& set_value(Variant value) {
    value_ = std::move(value);
    Add(AttributeId::Value);
    return *this;
  }

 private:
  QualifiedName browse_name_;
  LocalizedText display_name_;
  NodeId data_type_id_;
  Variant value_;
};

} // namespace scada
