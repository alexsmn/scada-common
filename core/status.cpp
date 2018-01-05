#include "core/status.h"

namespace scada {

// static
Status Status::FromFullCode(unsigned full_code) {
  Status result(StatusCode::Bad);
  result.full_code_ = full_code;
  return result;
}

} // namespace scada

namespace {

struct Entry {
  scada::StatusCode code;
  const char* error_string;
  const wchar_t* localized_error_string;
};

const Entry kEntries[] = {
    { scada::StatusCode::Good, "Good", L"Операция выполнена успешно" },
    { scada::StatusCode::Good_Pending, "Good_Pending", L"Операция выполняется" },
    { scada::StatusCode::Uncertain_StateWasNotChanged, "Uncertain_StateWasNotChanged", L"Блокировка не была изменена" },
    { scada::StatusCode::Bad, "Bad", L"Ошибка" },
    { scada::StatusCode::Bad_WrongLoginCredentials, "Bad_WrongLoginCredentials", L"Неверное имя пользователя или пароль" },
    { scada::StatusCode::Bad_UserIsAlreadyLoggedOn, "Bad_UserIsAlreadyLoggedOn", L"Сессия данного пользователя уже установлена" },
    { scada::StatusCode::Bad_UnsupportedProtocolVersion, "Bad_UnsupportedProtocolVersion", L"Версия протокола не поддерживается" },
    { scada::StatusCode::Bad_ObjectIsBusy, "Bad_ObjectIsBusy", L"В данный момент выполняется другая команда" },
    { scada::StatusCode::Bad_WrongNodeId, "Bad_WrongNodeId", L"Неправильный идентификатор узла" },
    { scada::StatusCode::Bad_WrongDeviceId, "Bad_WrongDeviceId", L"Неправильный идентификатор устройства" },
    { scada::StatusCode::Bad_Disconnected, "Bad_Disconnected", L"Соединение не установлено" },
    { scada::StatusCode::Bad_SessionForcedLogoff, "Bad_SessionForcedLogoff", L"Сессия разорвана из-за повторного подключения данного пользователя" },
    { scada::StatusCode::Bad_Timeout, "Bad_Timeout", L"Операция прервана по истечении времени ожидания" },
    { scada::StatusCode::Bad_CantDeleteDependentNode, "Bad_CantDeleteDependentNode", L"Невозможно удалить объект из-за наличия зависимых объектов" },
    { scada::StatusCode::Bad_ServerWasShutDown, "Bad_ServerWasShutDown", L"Сессия разорвана из-за остановки сервера" },
    { scada::StatusCode::Bad_WrongMethodId, "Bad_WrongMethodId", L"Команда не поддерживается данным объектом" },
    { scada::StatusCode::Bad_CantDeleteOwnUser, "Bad_CantDeleteOwnUser", L"Невозможно удалить пользователя из открытой им сессии" },
    { scada::StatusCode::Bad_DuplicateNodeId, "Bad_DuplicateNodeId", L"Объект с таким идентификатором уже существует" },
    { scada::StatusCode::Bad_UnsupportedFileVersion, "Bad_UnsupportedFileVersion", L"Версия файла не поддерживается" },
    { scada::StatusCode::Bad_WrongTypeId, "Bad_WrongTypeId", L"Неправильный тип объекта" },
    { scada::StatusCode::Bad_WrongParentId, "Bad_WrongParentId", L"Неправильный идентификатор родительского объекта" },
    { scada::StatusCode::Bad_SessionIsLoggedOff, "Bad_SessionIsLoggedOff", L"Авторизация не выполнена" },
    { scada::StatusCode::Bad_WrongSubscriptionId, "Bad_WrongSubscriptionId", L"Неправильный номер подписки" },
    { scada::StatusCode::Bad_WrongIndex, "Bad_WrongIndex", L"Неправильный индекс" },
    { scada::StatusCode::Bad_Iec60870UnknownType, "Bad_IecUnknownType", L"Неправильный тип ASDU протокола МЭК-60870" },
    { scada::StatusCode::Bad_Iec60870UnknownCot, "Bad_IecUnknownCot", L"Неправильная причина передачи протокола МЭК-60870" },
    { scada::StatusCode::Bad_Iec60870UnknownDevice, "Bad_IecUnknownDevice", L"Неправильный адрес устройства протокола МЭК-60870" },
    { scada::StatusCode::Bad_Iec60870UnknownAddress, "Bad_IecUnknownAddress", L"Неправильный адрес объекта протокола МЭК-60870" },
    { scada::StatusCode::Bad_Iec60870UnknownError, "Bad_IecUnknownError", L"Ошибка протокола МЭК-60870" },
    { scada::StatusCode::Bad_WrongCallArguments, "Bad_WrongCallArguments", L"Неправильные аргументы команды" },
    { scada::StatusCode::Bad_CantParseString, "Bad_CantParseString", L"Невозможно преобразовать строку в значение данного типа" },
    { scada::StatusCode::Bad_TooLongString, "Bad_TooLongString", L"Слишком длинная строка" },
    { scada::StatusCode::Bad_WrongPropertyId, "Bad_WrongPropertyId", L"Неправильный атрибут объекта" },
    { scada::StatusCode::Bad_WrongReferenceId, "Bad_WrongReferenceId", L"Неправильный тип ссылки" },
    { scada::StatusCode::Bad_WrongNodeClass, "Bad_WrongNodeClass", L"Неправильный класс узла" },
    { scada::StatusCode::Bad_Iec61850Error, "Bad_Iec61850Error", L"Ошибка протокола МЭК-61850" },
};

const Entry* FindEntry(scada::StatusCode status_code) {
  for (auto& entry : kEntries) {
    if (entry.code == status_code)
      return &entry;
  }
  return nullptr;
}

} // namespace

std::string ToString(scada::StatusCode status_code) {
  if (auto* entry = FindEntry(status_code))
    return entry->error_string;

  return IsGood(status_code) ? "OK" : "Error";
}

base::string16 ToString16(scada::StatusCode status_code) {
  if (auto* entry = FindEntry(status_code))
    return entry->localized_error_string;

  return IsGood(status_code) ? L"Операция выполнена успешно" : L"Ошибка";
}

std::string ToString(const scada::Status& status) {
  return ToString(status.code());
}

base::string16 ToString16(const scada::Status& status) {
  return ToString16(status.code());
}
