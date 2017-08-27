#pragma once

#include "base/base_export.h"
#include "base/strings/string16.h"

#include <windows.h>

namespace win_util {

BASE_EXPORT void InsertMenuItem(HMENU menu, int pos, UINT id, const base::char16* text, UINT state = 0);
BASE_EXPORT void InsertSubMenu(HMENU menu, int pos, const base::char16* text, HMENU submenu);
BASE_EXPORT void InsertMenuSeparator(HMENU menu, int pos);

BASE_EXPORT void RemoveConsequentMenuSeparators(HMENU menu);

// Find and remove menu item by command.
BASE_EXPORT int RemoveMenuCommand(HMENU menu, UINT command);

BASE_EXPORT base::string16 LoadResourceString(HMODULE module, unsigned id);

BASE_EXPORT base::string16 GetWindowText(HWND window_handle);
BASE_EXPORT int GetWindowInt(HWND window_handle);

BASE_EXPORT void SetWindowTextInt(HWND window_handle, int value);

BASE_EXPORT base::string16 GetListBoxItemText(HWND window_handle, int index);
BASE_EXPORT base::string16 GetComboBoxItemText(HWND window_handle, int index);

} // namespace win_util
