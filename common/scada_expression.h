#pragma once

#include "scada/data_value.h"

#include <string_view>
#include <vector>

namespace expression {
class Expression;
}

class ScadaExpression {
 public:
  ScadaExpression();
  ~ScadaExpression();

  struct Item {
    std::string name;
    scada::DataValue value;
  };

  // Calculate node count of expression tree.
  size_t GetNodeCount() const;
  // Check expression consists from single item and return item name.
  bool IsSingleName(std::string& item_name) const;
  static bool IsSingleName(std::string_view formula, std::string& item_name);

  void Parse(const char* buf);

  scada::Variant Calculate() const;

  std::string Format(bool aliases = false) const;

  void Clear();

  using ItemList = std::vector<Item>;

  ItemList items;

 protected:
  std::unique_ptr<expression::Expression> expression_;
};
