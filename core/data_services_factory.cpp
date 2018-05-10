#include "data_services_factory.h"

#include "base/strings/string_util.h"

namespace {

DataServicesInfoList& GetMutableDataServicesInfoList() {
  static DataServicesInfoList s_list;
  return s_list;
}

}  // namespace

const DataServicesInfoList& GetDataServicesInfoList() {
  return GetMutableDataServicesInfoList();
}

void RegisterDataServices(DataServicesInfo info) {
  GetMutableDataServicesInfoList().emplace_back(std::move(info));
}

bool EqualDataServicesName(base::StringPiece name1, base::StringPiece name2) {
  return base::EqualsCaseInsensitiveASCII(name1, name2);
}

bool CreateDataServices(base::StringPiece name,
                        const DataServicesContext& context,
                        DataServices& services) {
  for (auto& info : GetDataServicesInfoList()) {
    if (EqualDataServicesName(info.name, name))
      return info.factory_method(context, services);
  }
  return false;
}
