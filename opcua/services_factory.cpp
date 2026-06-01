#include "base/any_executor.h"
#include "opcua/client_session.h"
#include "scada/data_services_factory.h"
#include "scada/history_service.h"

namespace {

class UnsupportedHistoryService : public scada::HistoryService {
 public:
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override {
    co_return scada::HistoryReadRawResult{.status = scada::StatusCode::Bad};
  }

  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override {
    co_return scada::HistoryReadEventsResult{.status = scada::StatusCode::Bad};
  }
};

}  // namespace

namespace opcua {

bool CreateServices(const DataServicesContext& context,
                    DataServices& services) {
  auto session = std::make_shared<ClientSession>(context.executor,
                                                 context.transport_factory);
  auto history_service = std::make_shared<UnsupportedHistoryService>();
  services = {.session_service_ = session,
              .view_service_ = session,
              .node_management_service_ = session,
              .history_service_ = history_service,
              .attribute_service_ = session,
              .method_service_ = session,
              .monitored_item_service_ = session};
  return true;
}

}  // namespace opcua
