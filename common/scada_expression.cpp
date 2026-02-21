#include "common/scada_expression.h"

#include "base/strings/string_util.h"
#include "express/express.h"
#include "express/function.h"
#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/strings.h"

#include <stdexcept>

static const expression::LexemType LEX_TRUE = 't';
static const expression::LexemType LEX_FALSE = 'f';
static const expression::LexemType LEX_ITEM = 'i';
static const expression::LexemType LEX_UNITS = 'u';

namespace {

inline bool wp_isalpha(char ch) {
#pragma warning(push)
#pragma warning(disable : 4566)
  return ch && strchr(
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШ"
                   "ЩЬЫЪЭЮЯ",
                   ch) != 0;
#pragma warning(pop)
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

    if (expression::EqualsNoCase(s, "true"))
      return expression::Lexem{LEX_TRUE};
    if (expression::EqualsNoCase(s, "false"))
      return expression::Lexem{LEX_FALSE};

    // Parse name
    return expression::Lexem::String(expression::LEX_NAME, s);

  } else if (*buffer.buf == '{') {
    // Channel path.
    auto* start = buffer.buf;

    while (*buffer.buf && *buffer.buf != '}')
      ++buffer.buf;
    if (*buffer.buf != '}')
      throw std::runtime_error("no ending bracket");

    ++buffer.buf;

    std::string_view s{start, static_cast<size_t>(buffer.buf - start)};
    return expression::Lexem::String(expression::LEX_NAME, s);

  } else {
    return std::nullopt;
  }
}

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

struct ParserDelegate
    : public expression::BasicParserDelegate<expression::PolymorphicToken> {
  ParserDelegate(expression::Allocator& allocator, ScadaExpression& expression)
      : expression::BasicParserDelegate<
            expression::PolymorphicToken>{allocator},
        expression{expression} {}

  template <class Parser>
  expression::PolymorphicToken MakeCustomToken(const expression::Lexem& lexem,
                                               Parser& parser) {
    switch (lexem.lexem) {
      case LEX_TRUE:
      case LEX_FALSE:
        return expression::MakePolymorphicToken<BoolToken>(
            allocator_, lexem.lexem == LEX_TRUE);

      case expression::LEX_NAME: {
        // variable
        auto& item = expression.items.emplace_back();
        item.name = lexem._string;
        return expression::MakePolymorphicToken<ItemToken>(
            allocator_, expression, expression.items.size() - 1);
      }

      default:
        throw std::runtime_error{"Unexpected lexem"};
    }
  }

  const expression::BasicFunction<expression::PolymorphicToken>*
  FindBasicFunction(std::string_view name) {
    return expression::functions::FindDefaultFunction<
        expression::PolymorphicToken>(name);
  }

  ScadaExpression& expression;
};

}  // namespace

static bool _aliases;

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

ScadaExpression::ScadaExpression()
    : expression_{std::make_unique<expression::Expression>()} {}

ScadaExpression ::~ScadaExpression() = default;

void ScadaExpression::Parse(const char* buf) {
  ScadaLexerDelegate lexer_delegate;
  expression::Lexer lexer{buf, lexer_delegate, 0};
  expression::Allocator allocator;
  ParserDelegate parser_delegate{allocator, *this};
  expression::BasicParser<expression::Lexer, ParserDelegate> parser{
      lexer, parser_delegate};
  expression_->Parse(parser, allocator);
}

void ScadaExpression::Clear() {
  items.clear();
  expression_->Clear();
}

bool ScadaExpression::IsSingleName(std::string& item_name) const {
  if (GetNodeCount() != 1)
    return false;

  if (items.size() != 1)
    return false;

  bool result = true;
  auto& expr = const_cast<expression::Expression&>(*expression_);
  expr.Traverse(&Traversers::IsSingleName, &result);
  if (!result)
    return false;

  item_name = items.front().name;
  return true;
}

// static
bool ScadaExpression::IsSingleName(std::string_view formula,
                                   std::string& item_name) {
  const auto& buf = std::string{formula};
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

scada::Variant ScadaExpression::Calculate() const {
  for (size_t i = 0; i < items.size(); ++i)
    if (items[i].value.value.is_null())
      return scada::Variant();

  return static_cast<double>(expression_->Calculate());
}

std::string ScadaExpression::Format(bool aliases) const {
  _aliases = aliases;
  expression::FormatterDelegate formatter_delegate;
  return expression_->Format(formatter_delegate);
}

size_t ScadaExpression::GetNodeCount() const {
  size_t count = 0;
  expression_->Traverse(&Traversers::GetNodeCount, &count);
  return count;
}
