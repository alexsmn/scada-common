#pragma once

namespace scada {
class DataValue;
}

template <typename T>
struct TimedDataTraits;

template <typename T>
class BasicTimedDataView;

template <typename T>
class BasicTimedDataViewObserver;

using TimedDataView = BasicTimedDataView<scada::DataValue>;
using TimedDataViewObserver = BasicTimedDataViewObserver<scada::DataValue>;
