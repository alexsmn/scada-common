#include "core/status.h"

#include "base/strings/string_number_conversions.h"

namespace scada {

// static
Status Status::FromFullCode(unsigned full_code) {
  Status result(StatusCode::Bad);
  result.full_code_ = full_code;
  return result;
}

std::string Status::ToString() const {
  return scada::ToString(code());
}

std::string ToString(StatusCode status_code) {
  struct Entry {
    StatusCode code;
    const char* error_string;
  };

  const Entry kEntries[] = {
      { StatusCode::Good, "Операция выполнена успешно" },
      { StatusCode::Good_Pending, "Операция выполняется" },
      { StatusCode::Uncertain_StateWasNotChanged, "Блокировка не была изменена" },
      { StatusCode::Bad, "Ошибка" },
      { StatusCode::Bad_WrongLoginCredentials, "Неверное имя пользователя или пароль" },
      { StatusCode::Bad_UserIsAlreadyLoggedOn, "Сессия данного пользователя уже установлена" },
      { StatusCode::Bad_UnsupportedProtocolVersion, "Версия протокола не поддерживается" },
      { StatusCode::Bad_ObjectIsBusy, "В данный момент выполняется другая команда" },
      { StatusCode::Bad_WrongNodeId, "Неправильный идентификатор узла" },
      { StatusCode::Bad_WrongDeviceId, "Неправильный идентификатор устройства" },
      { StatusCode::Bad_Disconnected, "Соединение не установлено" },
      { StatusCode::Bad_SessionForcedLogoff, "Сессия разорвана из-за повторного подключения данного пользователя" },
      { StatusCode::Bad_Timeout, "Операция прервана по истечении времени ожидания" },
      { StatusCode::Bad_CantDeleteDependentNode, "Невозможно удалить объект из-за наличия зависимых объектов" },
      { StatusCode::Bad_ServerWasShutDown, "Сессия разорвана из-за остановки сервера" },
      { StatusCode::Bad_WrongMethodId, "Команда не поддерживается данным объектом" },
      { StatusCode::Bad_CantDeleteOwnUser, "Невозможно удалить пользователя из открытой им сессии" },
      { StatusCode::Bad_DuplicateNodeId, "Объект с таким идентификатором уже существует" },
      { StatusCode::Bad_UnsupportedFileVersion, "Версия файла не поддерживается" },
      { StatusCode::Bad_WrongTypeId, "Неправильный тип объекта" },
      { StatusCode::Bad_WrongParentId, "Неправильный идентификатор родительского объекта" },
      { StatusCode::Bad_SessionIsLoggedOff, "Авторизация не выполнена" },
      { StatusCode::Bad_WrongSubscriptionId, "Неправильный номер подписки" },
      { StatusCode::Bad_WrongIndex, "Неправильный индекс" },
      { StatusCode::Bad_IecUnknownType, "Неправильный тип ASDU протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownCot, "Неправильная причина передачи протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownDevice, "Неправильный адрес устройства протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownAddress, "Неправильный адрес объекта протокола МЭК-60870" },
      { StatusCode::Bad_IecUnknownError, "Ошибка протокола МЭК-60870" },
      { StatusCode::Bad_WrongCallArguments, "Неправильные аргументы команды" },
      { StatusCode::Bad_CantParseString, "Невозможно преобразовать строку в значение данного типа" },
      { StatusCode::Bad_TooLongString, "Слишком длинная строка" },
      { StatusCode::Bad_WrongPropertyId, "Неправильный атрибут объекта" },
      { StatusCode::Bad_WrongReferenceId, "Неправильный тип ссылки" },
      { StatusCode::Bad_WrongNodeClass, "Неправильный класс узла" },
  };

  for (auto& entry : kEntries) {
    if (entry.code == status_code)
      return entry.error_string;
  }

  return IsGood(status_code) ? "Операция выполнена успешно" : "Ошибка";
}

} // namespace scada