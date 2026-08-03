#pragma once

namespace scada {
class DataValue;
}

template <typename T>
struct TimedDataTraits;

// The non-owning, copyable time-series view (a time-aware span).
template <typename T>
class BasicTimedDataView;

using TimedDataView = BasicTimedDataView<scada::DataValue>;
