#pragma once

#include "core/data_value.h"
#include "express/express.h"
#include "express/lexer_delegate.h"
#include "express/parser_delegate.h"

#include <vector>

class ScadaExpression : private expression::LexerDelegate,
                        private expression::ParserDelegate,
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

  void Parse(const char* buf);

  scada::Variant Calculate() const;

  std::string Format(bool aliases = false) const;

  void Clear();

  ItemList items;

 protected:
  // expression::LexerDelegate
  virtual std::optional<expression::Lexem> ReadLexem(
      expression::ReadBuffer& buffer) override;

  // expression::ParserDelegate
  virtual expression::Token* CreateToken(expression::Allocator& allocator,
                                         const expression::Lexem& lexem,
                                         expression::Parser& parser) override;

  expression::Expression expression_;
};
