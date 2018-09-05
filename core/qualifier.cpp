#include "core/qualifier.h"

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
    text += L"Недост ";
  if (qualifier.backup())
    text += L"Резерв ";
  if (qualifier.offline())
    text += L"НетСвязи ";
  if (qualifier.manual())
    text += L"Ручной ";
  if (qualifier.misconfigured())
    text += L"НеСконф ";
  if (qualifier.simulated())
    text += L"Эмулирован ";
  if (qualifier.sporadic())
    text += L"Спорадика ";
  if (qualifier.stale())
    text += L"Устарел ";
  if (qualifier.failed())
    text += L"Ошибка ";
  return text;
}

std::ostream& operator<<(std::ostream& stream, scada::Qualifier qualifier) {
  return stream << ToString(qualifier);
}
