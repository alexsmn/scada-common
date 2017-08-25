#pragma once

#include "base/base_export.h"
#include "base/strings/string16.h"

#include <windows.h>

BASE_EXPORT bool DumpException(const base::char16* path, const EXCEPTION_POINTERS& exc);

BASE_EXPORT int DumpExceptionFilter(DWORD code, _EXCEPTION_POINTERS* exc);
