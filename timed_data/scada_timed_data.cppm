// scada.timed_data — named C++20 module facade over the common/timed_data
// headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). timed_data's CMake deps are all PRIVATE, but
// its headers textually expose scada/base types and the events EventObserver
// interface, so the facade export-imports scada.common and scada.events to
// keep `import scada.timed_data;` name-equivalent to the includes it
// replaces.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "timed_data/alias_timed_data.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/error_timed_data.h"
#include "timed_data/expression_timed_data.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_fake.h"
#include "timed_data/timed_data_fetcher.h"
#include "timed_data/timed_data_impl.h"
#include "timed_data/timed_data_buffer.h"
#include "timed_data/timed_data_buffer_fwd.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_service_factory.h"
#include "timed_data/timed_data_service_fake.h"
#include "timed_data/timed_data_service_impl.h"
#include "timed_data/timed_data_spec.h"
#include "timed_data/timed_data_util.h"
#include "timed_data/timed_data_view.h"
#include "timed_data/timed_data_view_fwd.h"
#include "timed_data/timed_data_view_observer.h"

export module scada.timed_data;

export import scada.common;
export import scada.events;

export {
  // timed_data.h (extern consts have external linkage - exportable)
  using ::kReadyCurrentTimeOnly;
  using ::kTimedDataCurrentOnly;
  using ::TimedData;

  // implementations
  using ::AliasTimedData;
  using ::BaseTimedData;
  using ::ErrorTimedData;
  using ::ExpressionTimedData;
  using ::FakeTimedData;
  using ::TimedDataImpl;

  // timed_data_context.h / timed_data_fetcher.h
  using ::CoroutineTimedDataContext;
  using ::TimedDataContext;
  using ::TimedDataFetcher;
  using ::TimedDataFetcherContext;

  // timed_data_observer.h / timed_data_property.h
  using ::PROPERTY_CURRENT;
  using ::PROPERTY_ITEM;
  using ::PROPERTY_TITLE;
  using ::PropertySet;
  using ::TimedDataObserver;
  using ::TimedDataPropertyID;

  // timed_data_service*.h
  using ::CreateTimedDataService;
  using ::FakeTimedDataService;
  using ::TimedDataService;
  using ::TimedDataServiceImpl;

  // timed_data_spec.h / timed_data_util.h
  using ::FindFirstGap;
  using ::FindLastGap;
  using ::GetReadyFrom;
  using ::TimedDataSpec;

  // timed_data_buffer*.h / timed_data_view*.h (internal:: dump helpers are not
  // exported)
  using ::BasicTimedDataBuffer;
  using ::BasicTimedDataView;
  using ::BasicTimedDataViewObserver;
  using ::RetentionPolicy;
  using ::TimedDataBuffer;
  using ::TimedDataTraits;
  using ::TimedDataView;
  using ::TimedDataViewObserver;
}  // export
