#pragma once

#include "core/data_value.h"
#include "express/express.h"
#include "express/express_delegate.h"

#include <vector>

namespace rt {

class ScadaExpression : private expression::ExpressionDelegate {
 public:
  struct Item {
    std::string name;
    scada::DataValue value;
  };

  typedef std::vector<Item> ItemList;

  ScadaExpression();

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
  // expression::Expression
  virtual expression::LexemData ReadLexem(
      expression::ReadBuffer& buffer) override;
  virtual int WriteLexem(const expression::LexemData& lexem_data,
                         expression::Parser& parser,
                         expression::Buffer& buffer) override;
  virtual void CalculateLexem(const expression::Buffer& buffer,
                              int pos,
                              expression::Lexem lexem,
                              expression::Value& value,
                              void* data) const override;
  virtual std::string FormatLexem(const expression::Buffer& buffer,
                                  int pos,
                                  expression::Lexem lexem) const;
  virtual void TraverseLexem(const expression::Expression& expr,
                             int pos,
                             expression::Lexem lexem,
                             expression::TraverseCallback callback,
                             void* param) const override;

  expression::Expression expression_;
};

}  // namespace rt
