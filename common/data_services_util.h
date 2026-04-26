#pragma once

#include "scada/data_services.h"

namespace data_services {

template <typename Service>
std::shared_ptr<Service> Unowned(Service& service) {
  return std::shared_ptr<Service>{std::shared_ptr<void>{}, &service};
}

inline bool HasServices(const DataServices& services) {
  return services.session_service_ || services.view_service_ ||
         services.node_management_service_ || services.history_service_ ||
         services.attribute_service_ || services.method_service_ ||
         services.monitored_item_service_ ||
         services.coroutine_session_service_ ||
         services.coroutine_view_service_ ||
         services.coroutine_node_management_service_ ||
         services.coroutine_history_service_ ||
         services.coroutine_attribute_service_ ||
         services.coroutine_method_service_;
}

inline DataServices FromUnownedServices(const scada::services& services) {
  return DataServices::FromUnownedServices(services);
}

}  // namespace data_services
