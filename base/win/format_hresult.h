#pragma once

#include "base/base_export.h"
#include "base/strings/string16.h"

#include <windows.h>

std::string BASE_EXPORT FormatHresult(HRESULT err);
base::string16 BASE_EXPORT FormatHresultUTF16(HRESULT err);
