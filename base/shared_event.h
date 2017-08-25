#include "base/win/scoped_handle.h"
#include "base/strings/string16.h"

#include <cassert>
#include <windows.h>

class SharedEvent {
 public:
  SharedEvent() {}

  bool Create(const base::char16* name) {
    handle_.Set(CreateEventW(NULL, FALSE, FALSE, name));
    return handle_.IsValid();
  }
  
  bool Open(const base::char16* name) {
    handle_.Set(OpenEventW(EVENT_MODIFY_STATE, TRUE, name));
    return handle_.IsValid();
  }
  
  void Set() {
    ::SetEvent(handle_.Get());
  }
  
  DWORD WaitFor(DWORD milliseconds) {
    return ::WaitForSingleObject(handle_.Get(), milliseconds);
  }
  
 private:
  base::win::ScopedHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(SharedEvent);
};
