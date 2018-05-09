#include "timed_data/scada_expression.h"

#include "express/function.h"
#include "express/parser.h"

#include <stdexcept>

namespace expression {

static const Lexem LEX_TRUE = 't';
static const Lexem LEX_FALSE = 'f';
static const Lexem LEX_ITEM = 'i';
static const Lexem LEX_NAME = 'm';
static const Lexem LEX_COMMA = ',';
static const Lexem LEX_UNITS = 'u';

}  // namespace expression

namespace rt {

static bool _aliases;

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

class Traversers {
 public:
  static bool GetNodeCount(const expression::Expression& expr,
                           expression::Lexem lexem,
                           void* param) {
    size_t& count = *static_cast<size_t*>(param);
    ++count;
    return true;
  }

  static bool IsSingleName(const expression::Expression& expr,
                           expression::Lexem lexem,
                           void* param) {
    bool& result = *static_cast<bool*>(param);
    result = lexem == expression::LEX_ITEM;
    return false;
  }
};

ScadaExpression::ScadaExpression() : expression_{*this} {}

void ScadaExpression::Parse(const char* buf) {
  expression_.Parse(buf);
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
  expr.TraverseNode(expr.root, &Traversers::IsSingleName, &result);
  if (!result)
    return false;

  item_name = items.front().name;
  return true;
}

expression::LexemData ScadaExpression::ReadLexem(
    expression::ReadBuffer& buffer) {
  expression::LexemData result = {};

  if (*buffer.buf == ',') {
    result.lexem = *buffer.buf++;

  } else if (*buffer.buf == '_' || wp_isalpha(*buffer.buf)) {
    const char* buf = buffer.buf;

    // read name
    const char* str = buffer.buf;
    do {
      buf++;
    } while (*buf == '_' || *buf == '.' || *buf == '!' || wp_isalnum(*buf));
    int strl = static_cast<int>(buf - buffer.buf);
    buffer.buf = buf;
    // Parse name
    base::StringPiece strp{str, strl};
    if (strp == "true") {
      result.lexem = expression::LEX_TRUE;
    } else if (strp == "false") {
      result.lexem = expression::LEX_FALSE;
    } else {
      result.lexem = expression::LEX_NAME;
      result._str = str;
      result._strl = strl;
    }

  } else if (*buffer.buf == '{') {
    // Channel path.
    result.lexem = expression::LEX_NAME;
    result._str = buffer.buf;

    while (*buffer.buf && *buffer.buf != '}')
      ++buffer.buf;
    if (*buffer.buf != '}')
      throw std::runtime_error("no ending bracket");

    ++buffer.buf;

    result._strl = buffer.buf - result._str;

  } else {
    throw std::runtime_error("incorrect lexem");
  }

  return result;
}

int ScadaExpression::WriteLexem(const expression::LexemData& lexem_data,
                                expression::Parser& parser,
                                expression::Buffer& buffer) {
  int pos = buffer.size;
  switch (parser.lexem_data_.lexem) {
    case expression::LEX_TRUE:
    case expression::LEX_FALSE:
      buffer << parser.lexem_data_.lexem;
      parser.ReadLexem();
      return pos;

    case expression::LEX_NAME: {
      std::string name(parser.lexem_data_._str, parser.lexem_data_._strl);
      parser.ReadLexem();
      if (parser.lexem_data_.lexem == expression::LEX_LP) {
        // function
        expression::Function* fun = FindFunction(name.c_str());
        if (!fun) {
          throw std::runtime_error(
              base::StringPrintf("function '%s' is not found", name.c_str())
                  .c_str());
        }
        // read parameters
        int params[256];
        unsigned char nparam = 0;
        parser.ReadLexem();
        if (parser.lexem_data_.lexem != expression::LEX_RP) {
          for (;;) {
            params[nparam++] = parser.expr_bin();
            if (parser.lexem_data_.lexem != expression::LEX_COMMA)
              break;
            parser.ReadLexem();
          }
          if (parser.lexem_data_.lexem != expression::LEX_RP)
            throw std::runtime_error("missing ')'");
        }
        parser.ReadLexem();
        if (fun->params != -1 && fun->params != nparam) {
          throw std::runtime_error(
              base::StringPrintf("%d parameters expected", fun->params)
                  .c_str());
        }
        // serialize
        pos = buffer.size;
        buffer << (char)expression::LEX_FUN << fun;
        if (fun->params == -1)
          buffer << nparam;
        buffer.write(params, nparam * sizeof(params[0]));
      } else {
        // variable
        items.push_back(Item());
        Item& item = items.back();
        // item.events = this;
        item.name = name;
        buffer << (char)expression::LEX_ITEM << (char)(items.size() - 1);
      }
      return pos;
    }

    case expression::LEX_DBL: {
      double val = parser.lexem_data_._double;
      int units = -1;
      if (*parser.buf == '%') {
        units = 0;
        parser.buf++;
      } else {
        parser.ReadLexem();
        /*if (parser.lexem == LEX_NAME) {
          units = find_units(parser._str, parser._strl);
        }*/
      }
      /*if (units != -1) {
        constant = false;
        parser.ReadLexem();
        *this << (char)LEX_UNITS << (char)units;
      }*/
      expression::WriteNumber(buffer, val);
      return pos;
    }

    default:
      return EXPR_DEF;
  }
}

void ScadaExpression::CalculateLexem(const expression::Buffer& buffer,
                                     int pos,
                                     expression::Lexem lexem,
                                     expression::Value& value,
                                     void* data) const {
  switch (lexem) {
    case expression::LEX_TRUE:
      value = 1.0;
      return;
    case expression::LEX_FALSE:
      value = 0.0;
      return;
    case expression::LEX_ITEM: {
      int i = buffer.read<char>(pos);
      value = ScadaToExpressionValue(items[i].value.value);
      return;
    }
    /*case LEX_UNITS:
      {
        int units = read<char>();
        _ASSERT(units >= 0 && units < _countof(::units));
        double factor = ::units[units].factor;
        calc(val);
        val *= factor;
        return;
      }*/
    default:
      throw std::runtime_error("CalculateLexem");
  }
}

std::string ScadaExpression::FormatLexem(const expression::Buffer& buffer,
                                         int pos,
                                         expression::Lexem lexem) const {
  switch (lexem) {
    case expression::LEX_TRUE:
      return "TRUE";
    case expression::LEX_FALSE:
      return "FALSE";
    case expression::LEX_ITEM: {
      int i = buffer.read<char>(pos);
      return items[i].name;
    }
    case expression::LEX_FUN: {
      expression::Function* fun = buffer.read<expression::Function*>(pos);
      return fun->Format(expression_, pos);
    }
    /*case LEX_UNITS:
      {
        int units = read<char>();
        _ASSERT(units >= 0 && units < _countof(::units));
        std::string str = format();
        str += ::units[units].units;
        return str;
      }*/
    default:
      throw std::runtime_error("FormatLexem");
  }
}

void ScadaExpression::TraverseLexem(const expression::Expression& expr,
                                    int pos,
                                    expression::Lexem lexem,
                                    expression::TraverseCallback callback,
                                    void* param) const {
  if (!callback(expr, lexem, param))
    return;

  if (lexem == expression::LEX_FUN) {
    expression::Function* fun = expr.buffer.read<expression::Function*>(pos);
    fun->Traverse(expr, pos, callback, param);
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
  auto& expr = const_cast<expression::Expression&>(expression_);
  return expr.FormatNode(expr.root);
}

size_t ScadaExpression::GetNodeCount() const {
  size_t count = 0;

  auto& expr = const_cast<expression::Expression&>(expression_);
  expr.TraverseNode(expr.root, &Traversers::GetNodeCount, &count);

  return count;
}

}  // namespace rt
