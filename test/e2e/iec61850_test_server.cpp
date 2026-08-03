#include "test/e2e/iec61850_test_server.h"

#include <iec61850_server.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace client::test {

Iec61850TestServer::Iec61850TestServer(int port)
    : port_{port}, thread_{[this] { Run(); }} {}

Iec61850TestServer::~Iec61850TestServer() {
  stopping_ = true;
  if (thread_.joinable())
    thread_.join();
}

void Iec61850TestServer::Run() {
  IedModel* model = IedModel_create("testmodel");

  LogicalDevice* l_device = LogicalDevice_create("SENSORS", model);
  LogicalNode* lln0 = LogicalNode_create("LLN0", l_device);

  CDC_ENS_create("Mod", reinterpret_cast<ModelNode*>(lln0), 0);
  CDC_ENS_create("Health", reinterpret_cast<ModelNode*>(lln0), 0);
  SettingGroupControlBlock_create(lln0, 1, 1);

  LogicalNode* ttmp1 = LogicalNode_create("TTMP1", l_device);
  DataObject* ttmp1_tmpsv =
      CDC_SAV_create("TmpSv", reinterpret_cast<ModelNode*>(ttmp1), 0, false);

  auto* temperature_value = reinterpret_cast<DataAttribute*>(
      ModelNode_getChild(reinterpret_cast<ModelNode*>(ttmp1_tmpsv),
                         "instMag.f"));
  auto* temperature_timestamp = reinterpret_cast<DataAttribute*>(
      ModelNode_getChild(reinterpret_cast<ModelNode*>(ttmp1_tmpsv), "t"));

  DataSet* data_set = DataSet_create("events", lln0);
  DataSetEntry_create(data_set, "TTMP1$MX$TmpSv$instMag$f", -1, nullptr);

  uint8_t report_options =
      RPT_OPT_SEQ_NUM | RPT_OPT_TIME_STAMP | RPT_OPT_REASON_FOR_INCLUSION;
  ReportControlBlock_create("events01", lln0, "events01", false, nullptr, 1,
                            TRG_OPT_DATA_CHANGED, report_options, 50, 0);
  ReportControlBlock_create("events02", lln0, "events02", false, nullptr, 1,
                            TRG_OPT_DATA_CHANGED, report_options, 50, 0);

  IedServer ied_server = IedServer_create(model);
  IedServer_start(ied_server, port_);

  if (!IedServer_isRunning(ied_server)) {
    failed_ = true;
    IedServer_destroy(ied_server);
    IedModel_destroy(model);
    return;
  }

  running_ = true;

  float value = 0.f;
  while (!stopping_) {
    IedServer_lockDataModel(ied_server);
    IedServer_updateUTCTimeAttributeValue(ied_server, temperature_timestamp,
                                          Hal_getTimeInMs());
    IedServer_updateFloatAttributeValue(ied_server, temperature_value, value);
    IedServer_unlockDataModel(ied_server);

    value += 0.1f;
    std::this_thread::sleep_for(100ms);
  }

  running_ = false;
  IedServer_stop(ied_server);
  IedServer_destroy(ied_server);
  IedModel_destroy(model);
}

}  // namespace client::test
