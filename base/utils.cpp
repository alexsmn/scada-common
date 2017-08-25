#include "base/format.h"
#include "core/status.h"
#include "base/utils.h"
#include "base/strings/string_util.h"

#ifdef OS_WIN
#include <windows.h>
#endif

#ifdef OS_WIN
HFONT CreateFont(LPCTSTR name, int height) {
  return ::CreateFont(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, name);
}
#endif

inline bool IsDigit(char ch) {
  const char* kDigitSymbols = "0123456789";
  return strchr(kDigitSymbols, ch) != 0;
}

int ScanEndingNumber(const char* str, size_t& len) {
  while (len && IsDigit(str[len - 1]))
    --len;
  return ParseWithDefault(str + len, -1);
}

int HumanCompareText(const char* left, const char* right) {
  size_t left_len = strlen(left);
  size_t right_len = strlen(right);
  
  // Skip identical non-digit endings.
  while (left_len && right_len && left[left_len - 1] == right[right_len - 1] &&
         !IsDigit(left[left_len - 1])) {
   --left_len;
   --right_len;
 }
  
  int left_value = ScanEndingNumber(left, left_len);
  int right_value = ScanEndingNumber(right, right_len);

  if (left_len != right_len)
     return base::CompareCaseInsensitiveASCII(left, right);

  int res = base::CompareCaseInsensitiveASCII(
      base::StringPiece(left, left_len),
      base::StringPiece(right, left_len));
  if (res != 0)
     return res;

  return left_value - right_value;
}
