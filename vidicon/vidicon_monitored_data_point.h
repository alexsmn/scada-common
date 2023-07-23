#pragma once

#include "scada/data_value.h"
#include "scada/monitored_item.h"
#include "vidicon/TeleClient.h"

#include <atlbase.h>
#include <atlcom.h>
#include <wrl/client.h>

class VidiconMonitoredDataPoint
    : public scada::MonitoredItem,
      public IDispEventImpl<1,
                            VidiconMonitoredDataPoint,
                            &DIID__IDataPointEvents,
                            &LIBID_TeleClientLib,
                            0xFFFF,
                            0xFFFF> {
 public:
  explicit VidiconMonitoredDataPoint(Microsoft::WRL::ComPtr<IDataPoint> point);
  ~VidiconMonitoredDataPoint();

  BEGIN_SINK_MAP(VidiconMonitoredDataPoint)
  SINK_ENTRY_EX(1, DIID__IDataPointEvents, 1, OnDataChange)  // OnStateChange
  SINK_ENTRY_EX(1, DIID__IDataPointEvents, 2, OnDataChange)  // OnDataChange
  END_SINK_MAP()

  // scada::MonitoredItem
  virtual void Subscribe(scada::MonitoredItemHandler handler) override;

 private:
  scada::DataValue GetDataValue() const;

  void __stdcall OnDataChange();

  Microsoft::WRL::ComPtr<IDataPoint> point_;

  scada::DataChangeHandler data_change_handler_;
};
