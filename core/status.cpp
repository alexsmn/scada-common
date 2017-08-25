#include "core/status.h"

#include "base/strings/string_number_conversions.h"

namespace scada {

// static
Status Status::FromFullCode(unsigned full_code) {
  Status result(StatusCode::Bad);
  result.full_code_ = full_code;
  return result;
}

base::string16 Status::ToString16() const {
  return scada::ToString16(code());
}

std::string ToString(StatusCode status_code) {
  return base::IntToString(static_cast<int>(status_code));
}

base::string16 ToString16(StatusCode status_code) {
  struct Entry {
    StatusCode code;
    const wchar_t* error_string;
  };

  const Entry kEntries[] = {
      { StatusCode::Good, L"Операция выполнена успешно" },
      { StatusCode::Good_Pending, L"Операция выполняется" },
      { StatusCode::Uncertain_StateWasNotChanged, L"Блокировка не была изменена" },
      { StatusCode::Bad, L"Ошибка" },
      { StatusCode::Bad_WrongLoginCredentials, L"Неверное имя пользователя или пароль" },
      { StatusCode::Bad_UserIsAlreadyLoggedOn, L"Сессия данного пользователя уже установлена" },
      { StatusCode::Bad_UnsupportedProtocolVersion, L"Версия протокола не поддерживается" },
      { StatusCode::Bad_ObjectIsBusy, L"В данный момент выполняется другая команда" },
      { StatusCode::Bad_WrongNodeId, L"Неправильный идентификатор" },
      { StatusCode::Bad_WrongDeviceId, L"Неправильный идентификатор устройства" },
      { StatusCode::Bad_Disconnected, L"Соединение не установлено" },
      { StatusCode::Bad_SessionForcedLogoff, L"Сессия разорвана из-за повторного подключения данного пользователя" },
      { StatusCode::Bad_Timeout, L"Операция прервана по истечении времени ожидания" },
      { StatusCode::Bad_CantDeleteDependentNode, L"Невозможно удалить объект из-за наличия зависимых объектов" },
      { StatusCode::Bad_ServerWasShutDown, L"Сессия разорвана из-за остановки сервера" },
      { StatusCode::Bad_WrongMethodId, L"Команда не поддерживается данным объектом" },
      { StatusCode::Bad_CantDeleteOwnUser, L"Невозможно удалить пользователя из открытой им сессии" },
      { StatusCode::Bad_DuplicateNodeId, L"Объект с таким идентификатором уже существует" },
      { StatusCode::Bad_UnsupportedFileVersion, L"Версия файла не поддерживается" },
      { StatusCode::Bad_WrongTypeId, L"Неправильный тип объекта" },
      { StatusCode::Bad_WrongParentId, L"Неправильный идентификатор родительского объекта" },
      { StatusCode::Bad_SessionIsLoggedOff, L"Авторизация не выполнена" },
      { StatusCode::Bad_WrongSubscriptionId, L"Неправильный номер подписки" },
      { StatusCode::Bad_WrongIndex, L"Неправильный индекс" },
      { StatusCode::Bad_IecUnknownType, L"Неправильный тип ASDU протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownCot, L"Неправильная причина передачи протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownDevice, L"Неправильный адрес устройства протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownAddress, L"Неправильный адрес объекта протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownError, L"Ошибка протокола МЭК-60870" },
      { StatusCode::Bad_WrongCallArguments, L"Неправильные аргументы команды" },
      { StatusCode::Bad_CantParseString, L"Невозможно преобразовать строку в значение данного типа" },
      { StatusCode::Bad_TooLongString, L"Слишком длинная строка" },
      { StatusCode::Bad_WrongPropertyId, L"Неправильный атрибут объекта" },
      { StatusCode::Bad_WrongReferenceId, L"Неправильный тип ссылки" },
      { StatusCode::Bad_WrongNodeClass, L"Неправильный класс узла" },
  };

  for (auto& entry : kEntries) {
    if (entry.code == status_code)
      return entry.error_string;
  }

  return IsGood(status_code) ? L"Операция выполнена успешно" : L"Ошибка";
}

} // namespace scada