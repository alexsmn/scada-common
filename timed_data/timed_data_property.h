#pragma once

namespace rt {

// Note: Changing of PROPERTY_ITEM means change of PROPERTY_TITLE.
enum TimedDataPropertyID { PROPERTY_TITLE   = 0x0001,
                           PROPERTY_ITEM    = 0x0002,
                           PROPERTY_CURRENT = 0x0004 };

class PropertySet {
 public:
  explicit PropertySet(unsigned mask)
      : mask_(mask) {
  };
 
  bool is_property_changed(TimedDataPropertyID property_id) const {
    return (mask_ & property_id) != 0;
  }
  bool is_current_changed() const { return is_property_changed(PROPERTY_CURRENT); }
  bool is_item_changed() const { return is_property_changed(PROPERTY_ITEM); }
  bool is_title_changed() const { return is_property_changed(PROPERTY_TITLE); }

 private:
  unsigned mask_;
};

} // namespace rt
