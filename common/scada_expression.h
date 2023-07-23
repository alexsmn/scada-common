#pragma once

#include "scada/data_value.h"
#include "express/express.h"

#include <string_view>
#include <vector>

class ScadaExpression {
 public:
  struct Item {
    std::string name;
    scada::DataValue value;
  };

  typedef std::vector<Item> ItemList;

  // Calculate node count of expression tree.
  size_t GetNodeCount() const;
  // Check expression consists from single item and return item name.
  bool IsSingleName(std::string& item_name) const;
  static bool IsSingleName(std::string_view formula, std::string& item_name);

  void Parse(const char* buf);

  scada::Variant Calculate() const;

  std::string Format(bool aliases = false) const;

  void Clear();

  ItemList items;

 protected:
  expression::Expression expression_;
};
