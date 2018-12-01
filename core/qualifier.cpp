#include "core/qualifier.h"

#include "base/strings/utf_string_conversions.h"

std::string ToString(scada::Qualifier qualifier) {
  std::string text;
  if (qualifier.bad())
    text += 'B';
  if (qualifier.backup())
    text += 'R';
  if (qualifier.offline())
    text += 'O';
  if (qualifier.manual())
    text += 'M';
  if (qualifier.misconfigured())
    text += 'C';
  if (qualifier.simulated())
    text += 'E';
  if (qualifier.sporadic())
    text += 'S';
  if (qualifier.stale())
    text += 'T';
  if (qualifier.failed())
    text += 'F';
  return text;
}

base::string16 ToString16(scada::Qualifier qualifier) {
  base::string16 text;
  if (qualifier.bad())
    text += base::WideToUTF16(L"Недост ");
  if (qualifier.backup())
    text += base::WideToUTF16(L"Резерв ");
  if (qualifier.offline())
    text += base::WideToUTF16(L"НетСвязи ");
  if (qualifier.manual())
    text += base::WideToUTF16(L"Ручной ");
  if (qualifier.misconfigured())
    text += base::WideToUTF16(L"НеСконф ");
  if (qualifier.simulated())
    text += base::WideToUTF16(L"Эмулирован ");
  if (qualifier.sporadic())
    text += base::WideToUTF16(L"Спорадика ");
  if (qualifier.stale())
    text += base::WideToUTF16(L"Устарел ");
  if (qualifier.failed())
    text += base::WideToUTF16(L"Ошибка ");
  return text;
}

std::ostream& operator<<(std::ostream& stream, scada::Qualifier qualifier) {
  return stream << ToString(qualifier);
}
