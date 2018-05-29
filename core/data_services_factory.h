#pragma once

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_piece.h"
#include "core/data_services.h"

#include <functional>
#include <memory>
#include <vector>

namespace base {
class SequencedTaskRunner;
}

namespace net {
class TransportFactory;
}

class Logger;

struct DataServicesContext {
  std::shared_ptr<Logger> logger;
  scoped_refptr<base::SequencedTaskRunner> task_runner;
  net::TransportFactory& transport_factory;
};

using DataServicesFactoryMethod =
    std::function<bool(const DataServicesContext& context,
                       DataServices& services)>;

struct DataServicesInfo {
  std::string name;
  base::string16 display_name;
  DataServicesFactoryMethod factory_method;
};

void RegisterDataServices(DataServicesInfo info);

using DataServicesInfoList = std::vector<DataServicesInfo>;

const DataServicesInfoList& GetDataServicesInfoList();

bool EqualDataServicesName(base::StringPiece name1, base::StringPiece name2);

#define REGISTER_DATA_SERVICES(name, display_name, factory_method) \
  static bool factory_method##_registered = [] {                   \
    RegisterDataServices({name, display_name, factory_method});    \
    return true;                                                   \
  }();

bool CreateDataServices(base::StringPiece name,
                        const DataServicesContext& context,
                        DataServices& services);
