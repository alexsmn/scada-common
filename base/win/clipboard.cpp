#include "clipboard.h"

#include <cassert>
#include <windows.h>

static HGLOBAL CreateHGlobal(const void* data, size_t size) {
  assert(size > 0);

  HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, size);
  if (!global)
    return NULL;
    
  void* ptr = GlobalLock(global);
  if (!ptr) {
    GlobalFree(global);
    return NULL;
  }
  
  memcpy(ptr, data, size);
  GlobalUnlock(global);
  
  return global;
}

static bool GetHGlobalData(HGLOBAL global, char*& data, size_t& size) {
  size = GlobalSize(global);
  if (!size)
    return false;
    
  void* ptr = GlobalLock(global);
  if (!ptr)
    return false;
  
  data = new char[size];
  memcpy(data, ptr, size);

  GlobalUnlock(global);

  return true;
}

// Clipboard

Clipboard::Clipboard() {
}

Clipboard::~Clipboard() {
  Close();
}

bool Clipboard::Open() {
  if (opened_)
    return true;

  if (!OpenClipboard(NULL))
    return false;

  opened_ = true;
  return true;
}

void Clipboard::Close() {
  if (!opened_)
    return;

  CloseClipboard();
  opened_ = false;
}

bool Clipboard::SetData(unsigned format, const void* data, size_t size) {
  assert(data);
  assert(size > 0);
  
  if (!Open())
    return false;
  
  HGLOBAL global = CreateHGlobal(data, size);
  if (!global)
    return false;

  if (!SetClipboardData(format, global)) {
    GlobalFree(global);
    return false;
  }

  return true;
}

bool Clipboard::GetData(unsigned format, char*& data, size_t& size) {
  if (!Open())
    return false;
  
  HGLOBAL global = GetClipboardData(format);
  if (!global)
    return false;

  // NOTE: No need for GlobalFree() here.
  return GetHGlobalData(global, data, size);
}

bool Clipboard::SetText(const std::string& text) {
  return SetData(CF_TEXT, text.c_str(), text.length() + 1); 
}

bool Clipboard::SetText(const base::string16& text) {
  return SetData(CF_UNICODETEXT, text.c_str(), (text.length() + 1) * 2);
}
