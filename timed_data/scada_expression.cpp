#include "timed_data/scada_expression.h"

#include "express/function.h"
#include "express/parser.h"

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

Value ScadaToExpressionValue(const scada::Variant& value) {
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
      return Value();
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

bool ScadaExpression::IsSingleName(std::string& item_name) const {
  if (GetNodeCount() != 1)
    return false;

  if (items.size() != 1)
    return false;

  bool result = true;
  ScadaExpression* expr = const_cast<ScadaExpression*>(this);
  expr->TraverseNode(root, &Traversers::IsSingleName, &result);
  if (!result)
    return false;

  item_name = items.front().name;
  return true;
}

void ScadaExpression::Parse(const char* formula) {
  __super::Parse(formula);
}

void ScadaExpression::ReadLexem(expression::Parser& parser) {
  if (*parser.buf == ',') {
    parser.lexem = *parser.buf++;

  } else if (*parser.buf == '_' || wp_isalpha(*parser.buf)) {
    const char* buf = parser.buf;

    // read name
    const char* str = parser.buf;
    do {
      buf++;
    } while (*buf == '_' || *buf == '.' || *buf == '!' || wp_isalnum(*buf));
    int strl = static_cast<int>(buf - parser.buf);
    parser.buf = buf;
    // Parse name
    if (_strnicmp(str, "true", strl) == 0) {
      parser.lexem = expression::LEX_TRUE;
    } else if (_strnicmp(str, "false", strl) == 0) {
      parser.lexem = expression::LEX_FALSE;
    } else {
      parser.lexem = expression::LEX_NAME;
      parser._str = str;
      parser._strl = strl;
    }

  } else if (*parser.buf == '{') {
    // Channel path.
    parser.lexem = expression::LEX_NAME;
    parser._str = parser.buf;

    while (*parser.buf && *parser.buf != '}')
      ++parser.buf;
    if (*parser.buf != '}')
      throw std::exception();

    ++parser.buf;

    parser._strl = parser.buf - parser._str;

  } else {
    throw std::exception("incorrect lexem", 1);
  }
}

int ScadaExpression::WriteLexem(expression::Parser& parser) {
  int pos = buffer.size;
  switch (parser.lexem) {
    case expression::LEX_TRUE:
    case expression::LEX_FALSE:
      buffer << parser.lexem;
      parser.ReadLexem();
      return pos;

    case expression::LEX_NAME: {
      std::string name(parser._str, parser._strl);
      parser.ReadLexem();
      if (parser.lexem == expression::LEX_LP) {
        // function
        expression::Function* fun = find_function(name.c_str());
        if (!fun) {
          throw std::exception(
              base::StringPrintf("function '%s' is not found", name.c_str())
                  .c_str());
        }
        // read parameters
        int params[256];
        unsigned char nparam = 0;
        parser.ReadLexem();
        if (parser.lexem != expression::LEX_RP) {
          for (;;) {
            params[nparam++] = parser.expr_bin();
            if (parser.lexem != expression::LEX_COMMA)
              break;
            parser.ReadLexem();
          }
          if (parser.lexem != expression::LEX_RP)
            throw std::exception("missing ')'", 1);
        }
        parser.ReadLexem();
        if (fun->params != -1 && fun->params != nparam) {
          throw std::exception(
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
      double val = parser._double;
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

void ScadaExpression::CalculateLexem(int pos,
                                     expression::Lexem lexem,
                                     Value& value,
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
      throw std::exception("CalculateLexem", 1);
  }
}

std::string ScadaExpression::FormatLexem(int pos,
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
      return fun->Format(*this, pos);
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
      throw std::exception("FormatLexem", 1);
  }
}

void ScadaExpression::TraverseLexem(int pos,
                                    expression::Lexem lexem,
                                    expression::TraverseCallback callback,
                                    void* param) const {
  if (!callback(*this, lexem, param))
    return;

  if (lexem == expression::LEX_FUN) {
    expression::Function* fun = buffer.read<expression::Function*>(pos);
    fun->Traverse(*this, pos, callback, param);
  }
}

scada::Variant ScadaExpression::Calculate() const {
  for (size_t i = 0; i < items.size(); ++i)
    if (items[i].value.value.is_null())
      return scada::Variant();

  return static_cast<double>(__super::Calculate());
}

std::string ScadaExpression::Format(bool aliases) const {
  _aliases = aliases;
  ScadaExpression* expr = const_cast<ScadaExpression*>(this);
  return ((expression::Expression*)expr)->FormatNode(root);
}

size_t ScadaExpression::GetNodeCount() const {
  size_t count = 0;

  ScadaExpression* expr = const_cast<ScadaExpression*>(this);
  expr->TraverseNode(root, &Traversers::GetNodeCount, &count);

  return count;
}

}  // namespace rt
