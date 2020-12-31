#include "vidicon_monitored_data_point.h"

#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "base/win/scoped_variant.h"
#include "core/qualifier.h"

#include <opcda.h>

namespace base {

inline int64_t InSeconds(const SYSTEMTIME& t) {
  FILETIME ft = {};
  ::SystemTimeToFileTime(&t, &ft);
  return reinterpret_cast<LARGE_INTEGER&>(ft).QuadPart;
}

Time ToTime(DATE time) {
  if (std::abs(time) < std::numeric_limits<DATE>::epsilon())
    return {};

  SYSTEMTIME system_time{};
  ::VariantTimeToSystemTime(time, &system_time);

  SYSTEMTIME utc{};
  ::TzSpecificLocalTimeToSystemTime(nullptr, &system_time, &utc);
  utc.wMilliseconds =
      static_cast<WORD>(std::round((time - floor(time)) * 24 * 60 * 60 * 1000));

  Time result;
  Time::Exploded exploded{
      system_time.wYear,        system_time.wMonth, 0,
      system_time.wDay,         system_time.wHour,  system_time.wSecond,
      system_time.wMilliseconds};
  Time::FromUTCExploded(exploded, &result);
  return result;
}

}  // namespace base

namespace scada {

Variant ToVariant(const VARIANT& value) {
  switch (value.vt) {
    case VT_BOOL:
      return !!value.boolVal;
    case VT_BSTR:
      return base::SysWideToNativeMB(value.bstrVal);
    case VT_I2:
      return value.iVal;
    case VT_I4:
      return value.intVal;
    case VT_UI2:
      return value.uiVal;
    case VT_UI4:
      return static_cast<int>(value.uintVal);
    case VT_R4:
      return value.fltVal;
    case VT_R8:
      return value.dblVal;
    default:
      return {};
  }
}

scada::Qualifier OpcQualityToQualifier(unsigned quality) {
  // TODO:
  return (quality & OPC_QUALITY_MASK) != OPC_QUALITY_GOOD;
}

}  // namespace scada

VidiconMonitoredDataPoint::VidiconMonitoredDataPoint(
    Microsoft::WRL::ComPtr<IDataPoint> point)
    : point_(std::move(point)) {
  assert(point_);
  DispEventAdvise(point_.Get(), &DIID__IDataPointEvents);
}

VidiconMonitoredDataPoint::~VidiconMonitoredDataPoint() {
  DispEventUnadvise(point_.Get(), &DIID__IDataPointEvents);
}

void VidiconMonitoredDataPoint::Subscribe() {
  subscribed_ = true;
  ForwardData(GetDataValue());
}

scada::DataValue VidiconMonitoredDataPoint::GetDataValue() const {
  base::win::ScopedVariant value;
  ULONG quality = 0;
  DATE time = 0;
  point_->get_Value(value.Receive());
  point_->get_Quality(&quality);
  point_->get_Time(&time);

  auto timestamp = base::ToTime(time);
  return {scada::ToVariant(value), scada::OpcQualityToQualifier(quality),
          timestamp, timestamp};
}

void VidiconMonitoredDataPoint::OnDataChange() {
  if (subscribed_)
    ForwardData(GetDataValue());
}
