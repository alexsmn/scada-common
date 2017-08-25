#pragma once

#include "core/data_value.h"
#include "express/express.h"

#include <vector>

namespace rt {

class ScadaExpression : protected expression::Expression {
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

  void Clear() {
    items.clear();
    __super::Clear();
  }

  void Parse(const char* formula);

  scada::Variant Calculate() const;

  std::string Format(bool aliases = false) const;

  ItemList items;

 protected:
  // expression::Expression
  virtual void ReadLexem(expression::Parser& parser);
  virtual int WriteLexem(expression::Parser& parser);
  virtual void CalculateLexem(int pos, expression::Lexem lexem, Value& value,
                              void* data) const;
  virtual std::string FormatLexem(int pos, expression::Lexem lexem) const;
  virtual void TraverseLexem(int pos, expression::Lexem lexem,
                             expression::TraverseCallback callback,
                             void* param) const;
};

} // namespace rt
