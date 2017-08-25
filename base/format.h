#pragma once

#include "base/strings/stringprintf.h"
#include "base/strings/string_piece.h"
#include "base/strings/string16.h"
#include "build/build_config.h"

#include <cassert>
#include <exception>
#include <stdarg.h>

std::string Format(double value);
base::string16 WideFormat(double value);

std::string Format(float value);
base::string16 WideFormat(float value);

std::string Format(int value);
base::string16 WideFormat(int value);

std::string Format(unsigned int value);
base::string16 WideFormat(unsigned int value);

std::string Format(int64_t value);
base::string16 WideFormat(int64_t value);

std::string Format(uint64_t value);
base::string16 WideFormat(uint64_t value);

std::string Format(bool value);
base::string16 WideFormat(bool value);

std::string Format(unsigned char value);
base::string16 WideFormat(unsigned char value);

std::string Format(unsigned short value);
base::string16 WideFormat(unsigned short value);

std::string Format(base::StringPiece value);
base::string16 WideFormat(base::StringPiece value);

std::string Format(base::StringPiece16 value);
base::string16 WideFormat(base::StringPiece16 value);

template<typename T>
bool Parse(const base::StringPiece& str, T& value);
template<typename T>
bool Parse(const base::StringPiece16& str, T& value);

template<typename T>
inline T ParseWithDefault(const base::StringPiece& str, T def) {
  T value;
  return Parse(str, value) ? value : def;
}

template<typename T>
inline T ParseWithDefault(const base::StringPiece16& str, T def) {
  T value;
  return Parse(str, value) ? value : def;
}

template<typename T>
inline T ParseT(const base::StringPiece& str) {
  T value;
  if (!Parse<T>(str, value))
    throw std::exception();
  return value;
}

template<typename T>
inline T ParseT(const base::StringPiece16& str) {
  T value;
  if (!Parse<T>(str, value))
    throw std::exception();
  return value;
}

std::string FormatHexBuffer(const void* buf, size_t len);
