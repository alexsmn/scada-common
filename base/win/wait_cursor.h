#pragma once

#include "base/basictypes.h"

#include <windows.h>

class WaitCursor {
 public:
  WaitCursor()
      : old_cursor_(SetCursor(LoadCursor(NULL, IDC_WAIT))) {
  }
  
  ~WaitCursor() {
    SetCursor(old_cursor_);
  }

 private:
  HCURSOR old_cursor_;

  DISALLOW_COPY_AND_ASSIGN(WaitCursor);
};
