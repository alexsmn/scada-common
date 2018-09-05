#include "base/win/win_util2.h"

#include "base/format.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

#include <shellapi.h>
#include <memory>

namespace win_util {

void InsertMenuItem(HMENU menu, int pos, UINT id, LPCTSTR text, UINT state) {
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE;
  mii.fType = MFT_STRING;
  mii.fState = state;
  mii.wID = id;
  mii.dwTypeData = (LPTSTR)text;
  InsertMenuItem(menu, pos, TRUE, &mii);
}

void InsertSubMenu(HMENU menu,
                   int pos,
                   const base::char16* text,
                   HMENU submenu) {
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
  mii.fType = MFT_STRING;
  mii.dwTypeData = (LPTSTR)text;
  mii.hSubMenu = submenu;
  InsertMenuItem(menu, pos, TRUE, &mii);
}

void InsertMenuSeparator(HMENU menu, int pos) {
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  InsertMenuItem(menu, pos, TRUE, &mii);
}

void RemoveConsequentMenuSeparators(HMENU menu) {
  if (GetMenuItemCount(menu) == 0)
    return;

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE;

  int count;
  while ((count = GetMenuItemCount(menu)) > 0 &&
         GetMenuItemInfo(menu, 0, TRUE, &mii) && mii.fType == MFT_SEPARATOR) {
    DeleteMenu(menu, 0, MF_BYPOSITION);
  }

  while ((count = GetMenuItemCount(menu)) > 0 &&
         GetMenuItemInfo(menu, count - 1, TRUE, &mii) &&
         mii.fType == MFT_SEPARATOR) {
    DeleteMenu(menu, count - 1, MF_BYPOSITION);
  }

  // Remove consequent separators.
  bool last_sep = false;
  for (int i = 0; i < GetMenuItemCount(menu);) {
    GetMenuItemInfo(menu, i, TRUE, &mii);
    if (mii.fType == MFT_SEPARATOR) {
      if (last_sep) {
        DeleteMenu(menu, i, MF_BYPOSITION);
        last_sep = false;
        continue;
      }
      last_sep = true;
    } else
      last_sep = false;
    i++;
  }
}

int RemoveMenuCommand(HMENU menu, UINT command) {
  int pos = -1;
  for (int i = 0; i < GetMenuItemCount(menu); i++)
    if (GetMenuItemID(menu, i) == command) {
      pos = i;
      break;
    }

  if (pos != -1)
    DeleteMenu(menu, pos, MF_BYPOSITION);
  return pos;
}

base::string16 LoadResourceString(HMODULE module, unsigned id) {
  wchar_t* buffer = NULL;
  int len = ::LoadStringW(module, id, reinterpret_cast<LPWSTR>(&buffer), 0);
  return base::string16(buffer, len);
}

base::string16 GetWindowText(HWND window_handle) {
  DCHECK(IsWindow(window_handle));
  int len = GetWindowTextLength(window_handle) + 1;
  base::string16 str;
  if (len > 1)
    ::GetWindowText(window_handle, base::WriteInto(&str, len), len);
  return str;
}

int GetWindowInt(HWND window_handle) {
  base::string16 str = GetWindowText(window_handle);
  int value;
  if (!Parse(str, value))
    throw E_INVALIDARG;
  return value;
}

void SetWindowTextInt(HWND window_handle, int value) {
  SetWindowText(window_handle, WideFormat(value).c_str());
}

base::string16 GetListBoxItemText(HWND window_handle, int index) {
  int len = SendMessage(window_handle, LB_GETTEXTLEN, index, 0L);
  if (len <= 0)
    return base::string16();

  std::unique_ptr<base::char16[]> text(new base::char16[len + 1]);
  int res = SendMessage(window_handle, LB_GETTEXT, index,
                        reinterpret_cast<LPARAM>(text.get()));
  if (res == LB_ERR)
    return base::string16();
  return base::string16(text.get(), text.get() + len);
}

base::string16 GetComboBoxItemText(HWND window_handle, int index) {
  int len = SendMessage(window_handle, CB_GETLBTEXTLEN, index, 0L);
  if (len <= 0)
    return base::string16();

  std::unique_ptr<base::char16[]> text(new base::char16[len + 1]);
  int res = SendMessage(window_handle, CB_GETLBTEXT, index,
                        reinterpret_cast<LPARAM>(text.get()));
  if (res == LB_ERR)
    return base::string16();
  return base::string16(text.get(), text.get() + len);
}

enum Version {
  VERSION_PRE_2000 = 0,  // Not supported
  VERSION_2000 = 1,      // Not supported
  VERSION_XP = 2,
  VERSION_SERVER_2003 = 3,  // Also includes Windows XP Professional x64 edition
  VERSION_VISTA = 4,
  VERSION_2008 = 5,
  VERSION_WIN7 = 6,
};

Version GetVersion() {
  static bool checked_version = false;
  static Version win_version = VERSION_PRE_2000;
  if (!checked_version) {
    OSVERSIONINFOEX version_info;
    version_info.dwOSVersionInfoSize = sizeof version_info;
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&version_info));
    if (version_info.dwMajorVersion == 5) {
      switch (version_info.dwMinorVersion) {
        case 0:
          win_version = VERSION_2000;
          break;
        case 1:
          win_version = VERSION_XP;
          break;
        case 2:
        default:
          win_version = VERSION_SERVER_2003;
          break;
      }
    } else if (version_info.dwMajorVersion == 6) {
      if (version_info.wProductType != VER_NT_WORKSTATION) {
        // 2008 is 6.0, and 2008 R2 is 6.1.
        win_version = VERSION_2008;
      } else {
        if (version_info.dwMinorVersion == 0) {
          win_version = VERSION_VISTA;
        } else {
          win_version = VERSION_WIN7;
        }
      }
    } else if (version_info.dwMajorVersion > 6) {
      win_version = VERSION_WIN7;
    }
    checked_version = true;
  }
  return win_version;
}

void OpenWithAssociatedProgram(const std::filesystem::path& path) {
  ShellExecute(NULL, NULL, path.c_str(), NULL, NULL, SW_SHOW);
}

}  // namespace win_util
