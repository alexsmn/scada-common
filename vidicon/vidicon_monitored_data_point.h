#pragma once

#include "base/win/scoped_comptr.h"
#include "core/data_value.h"
#include "core/monitored_item.h"
#include "vidicon/TeleClient.h"

#include <atlbase.h>
#include <atlcom.h>

class VidiconMonitoredDataPoint
    : public scada::MonitoredItem,
      public IDispEventImpl<1, VidiconMonitoredDataPoint, &DIID__IDataPointEvents, &LIBID_TeleClientLib, 0xFFFF, 0xFFFF> {
 public:
  explicit VidiconMonitoredDataPoint(base::win::ScopedComPtr<IDataPoint> point);
  ~VidiconMonitoredDataPoint();

  BEGIN_SINK_MAP(VidiconMonitoredDataPoint)
    SINK_ENTRY_EX(1, DIID__IDataPointEvents, 1, OnDataChange) // OnStateChange
    SINK_ENTRY_EX(1, DIID__IDataPointEvents, 2, OnDataChange) // OnDataChange
  END_SINK_MAP()

  // scada::MonitoredItem
  virtual void Subscribe() override;

 private:
  scada::DataValue GetDataValue() const;

  void __stdcall OnDataChange();

  base::win::ScopedComPtr<IDataPoint> point_;
  bool subscribed_ = false;
};
