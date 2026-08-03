#pragma once

namespace scada {
class DataValue;
}

template <typename T>
struct TimedDataTraits;

// The owning time-series container (was BasicTimedDataView).
template <typename T>
class BasicTimedDataBuffer;

// Observes mutations and readiness of a BasicTimedDataBuffer.
template <typename T>
class BasicTimedDataViewObserver;

using TimedDataBuffer = BasicTimedDataBuffer<scada::DataValue>;
using TimedDataViewObserver = BasicTimedDataViewObserver<scada::DataValue>;
