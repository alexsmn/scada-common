#pragma once

#include "base/strings/string_piece.h"
#include "core/data_value.h"
#include "express/express.h"
#include "express/parser_delegate.h"

#include <vector>

class ScadaExpression : private expression::ParserDelegate,
                        private expression::FormatterDelegate {
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
  static bool IsSingleName(base::StringPiece formula, std::string& item_name);

  void Parse(const char* buf);

  scada::Variant Calculate() const;

  std::string Format(bool aliases = false) const;

  void Clear();

  ItemList items;

 protected:
  // expression::ParserDelegate
  virtual expression::Token* CreateToken(expression::Allocator& allocator,
                                         const expression::Lexem& lexem,
                                         expression::Parser& parser) override;

  expression::Expression expression_;
};
