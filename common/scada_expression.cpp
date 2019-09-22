#include "common/scada_expression.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "express/function.h"
#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"

#include <stdexcept>

namespace expression {

static const LexemType LEX_TRUE = 't';
static const LexemType LEX_FALSE = 'f';
static const LexemType LEX_ITEM = 'i';
static const LexemType LEX_UNITS = 'u';

}  // namespace expression

namespace {

inline bool wp_isalpha(char ch) {
  return ch && strchr(
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШ"
                   "ЩЬЫЪЭЮЯ",
                   ch) != 0;
}

inline bool wp_isnum(char ch) {
  return '0' <= ch && ch <= '9';
}

inline bool wp_isalnum(char ch) {
  return wp_isalpha(ch) || wp_isnum(ch);
}

class ScadaLexerDelegate : public expression::LexerDelegate {
 public:
  // expression::LexerDelegate
  virtual std::optional<expression::Lexem> ReadLexem(
      expression::ReadBuffer& buffer) override;
};

std::optional<expression::Lexem> ScadaLexerDelegate::ReadLexem(
    expression::ReadBuffer& buffer) {
  if (*buffer.buf == '_' || wp_isalpha(*buffer.buf)) {
    const char* buf = buffer.buf;

    // read name
    const char* str = buffer.buf;
    do {
      buf++;
    } while (*buf == '_' || *buf == '.' || *buf == '!' || wp_isalnum(*buf));
    int strl = static_cast<int>(buf - str);
    buffer.buf = buf;

    std::string_view s{str, static_cast<size_t>(strl)};

    if (expression::EqualsNoCase(s, "true")) {
      expression::Lexem result = {};
      result.lexem = expression::LEX_TRUE;
      return result;
    }

    if (expression::EqualsNoCase(s, "false")) {
      expression::Lexem result = {};
      result.lexem = expression::LEX_FALSE;
      return result;
    }

    // Parse name
    expression::Lexem result = {};
    result.lexem = expression::LEX_NAME;
    result._string = s;

    return result;

  } else if (*buffer.buf == '{') {
    // Channel path.
    auto* start = buffer.buf;

    while (*buffer.buf && *buffer.buf != '}')
      ++buffer.buf;
    if (*buffer.buf != '}')
      throw std::runtime_error("no ending bracket");

    ++buffer.buf;

    expression::Lexem result = {};
    result.lexem = expression::LEX_NAME;
    result._string =
        std::string_view{start, static_cast<size_t>(buffer.buf - start)};
    return result;

  } else {
    return std::nullopt;
  }
}

}  // namespace

static bool _aliases;

expression::Value ScadaToExpressionValue(const scada::Variant& value) {
  switch (value.type()) {
    case scada::Variant::BOOL:
      return value.as_bool();
    case scada::Variant::INT32:
      return value.as_int32();
    case scada::Variant::INT64:
      return static_cast<int>(value.as_int64());
    case scada::Variant::DOUBLE:
      return value.as_double();
    case scada::Variant::STRING:
      return value.as_string().c_str();
    default:
      return {};
  }
}

class BoolToken : public expression::Token {
 public:
  explicit BoolToken(bool value) : value_{value} {}

  virtual expression::Value Calculate(void* data) const override {
    return value_ ? 1.0 : 0.0;
  }

  virtual void Traverse(expression::TraverseCallback callb,
                        void* param) const override {
    callb(this, param);
  }

  virtual void Format(const expression::FormatterDelegate& delegate,
                      std::string& str) const override {
    str.append(value_ ? "TRUE" : "FALSE");
  }

 private:
  const bool value_;
};

class ItemToken : public expression::Token {
 public:
  ItemToken(const ScadaExpression& expression, int index)
      : expression_{expression}, index_{index} {}

  virtual expression::Value Calculate(void* data) const override {
    const auto& item = expression_.items[index_];
    return ScadaToExpressionValue(item.value.value);
  }

  virtual void Traverse(expression::TraverseCallback callb,
                        void* param) const override {
    callb(this, param);
  }

  virtual void Format(const expression::FormatterDelegate& delegate,
                      std::string& str) const override {
    const auto& item = expression_.items[index_];
    str.append(item.name);
  }

 private:
  const ScadaExpression& expression_;
  const int index_;
};

class Traversers {
 public:
  static bool GetNodeCount(const expression::Token* token, void* param) {
    size_t& count = *static_cast<size_t*>(param);
    ++count;
    return true;
  }

  static bool IsSingleName(const expression::Token* token, void* param) {
    bool& result = *static_cast<bool*>(param);
    result = dynamic_cast<const ItemToken*>(token) != nullptr;
    return false;
  }
};

void ScadaExpression::Parse(const char* buf) {
  ScadaLexerDelegate lexer_delegate;
  expression_.Parse(buf, lexer_delegate, *this);
}

void ScadaExpression::Clear() {
  items.clear();
  expression_.Clear();
}

bool ScadaExpression::IsSingleName(std::string& item_name) const {
  if (GetNodeCount() != 1)
    return false;

  if (items.size() != 1)
    return false;

  bool result = true;
  auto& expr = const_cast<expression::Expression&>(expression_);
  expr.Traverse(&Traversers::IsSingleName, &result);
  if (!result)
    return false;

  item_name = items.front().name;
  return true;
}

// static
bool ScadaExpression::IsSingleName(base::StringPiece formula,
                                   std::string& item_name) {
  const auto& buf = formula.as_string();
  ScadaLexerDelegate delegate;
  expression::Lexer lexer{buf.c_str(), delegate, 0};
  try {
    const auto& lexem1 = lexer.ReadLexem();
    if (lexem1.lexem != expression::LEX_NAME)
      return false;
    const auto& lexem2 = lexer.ReadLexem();
    if (lexem2.lexem != expression::LEX_END)
      return false;
    item_name = lexem1._string;
  } catch (const std::runtime_error&) {
    return false;
  }
  return true;
}

expression::Token* ScadaExpression::CreateToken(
    expression::Allocator& allocator,
    const expression::Lexem& lexem,
    expression::Parser& parser) {
  switch (lexem.lexem) {
    case expression::LEX_TRUE:
    case expression::LEX_FALSE:
      return expression::CreateToken<BoolToken>(
          allocator, lexem.lexem == expression::LEX_TRUE);

    case expression::LEX_NAME: {
      // variable
      auto& item = items.emplace_back();
      item.name = lexem._string;
      return expression::CreateToken<ItemToken>(allocator, *this,
                                                items.size() - 1);
    }

    default:
      return nullptr;
  }
}

scada::Variant ScadaExpression::Calculate() const {
  for (size_t i = 0; i < items.size(); ++i)
    if (items[i].value.value.is_null())
      return scada::Variant();

  return static_cast<double>(expression_.Calculate());
}

std::string ScadaExpression::Format(bool aliases) const {
  _aliases = aliases;
  return expression_.Format(*this);
}

size_t ScadaExpression::GetNodeCount() const {
  size_t count = 0;
  expression_.Traverse(&Traversers::GetNodeCount, &count);
  return count;
}
