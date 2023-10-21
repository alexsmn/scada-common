#pragma once

namespace scada {
class DataValue;
}

struct DataValueTime;

template <typename T, typename K>
class BasicTimedDataView;

template <typename T>
class BasicTimedDataViewObserver;

using TimedDataView = BasicTimedDataView<scada::DataValue, DataValueTime>;
using TimedDataViewObserver = BasicTimedDataViewObserver<scada::DataValue>;
