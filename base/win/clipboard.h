#pragma once

#include "base/strings/string16.h"

class Clipboard {
 public:
  Clipboard();
  ~Clipboard();

  Clipboard(const Clipboard&) = delete;
  Clipboard& operator=(const Clipboard&) = delete;
 
  bool Open();
  void Close();
 
  bool SetData(unsigned format, const void* data, size_t size);
  bool GetData(unsigned format, char*& data, size_t& size);

  bool SetText(const std::string& text);
  bool SetText(const base::string16& text);
  
 private:
  bool opened_ = false;
};
