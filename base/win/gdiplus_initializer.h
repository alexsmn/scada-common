#pragma once

#include <algorithm>

using std::min;
using std::max;

#include <atlbase.h>
#include <gdiplus.h>

class GdiplusInitializer {
 public:
  GdiplusInitializer() {
    Gdiplus::GdiplusStartup(&token_, &input_, nullptr);
  }
  
  ~GdiplusInitializer() {
    Gdiplus::GdiplusShutdown(token_);
  }

  GdiplusInitializer(const GdiplusInitializer&) = delete;
  GdiplusInitializer& operator=(const GdiplusInitializer&) = delete;
  
 private:
  Gdiplus::GdiplusStartupInput input_;
  ULONG_PTR token_;
};
