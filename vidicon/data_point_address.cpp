#include "vidicon/data_point_address.h"

#include "base/format.h"
#include "base/utf_convert.h"

namespace vidicon {

std::optional<DataPointAddress> ParseDataPointAddress(std::wstring_view str) {
  if (str.starts_with(L"AE:")) {
    return std::nullopt;
  }

  if (str.starts_with(L"CF:")) {
    str = str.substr(3);
    unsigned object_id = 0;
    auto narrow = UtfConvert<char>(str);
    if (!Parse(narrow, object_id)) {
      return std::nullopt;
    }
    return DataPointAddress{.object_id = object_id};
  }

  if (str.starts_with(L"DA:")) {
    str = str.substr(3);
  }

  return DataPointAddress{.opc_address = std::wstring{str}};
}

}  // namespace vidicon