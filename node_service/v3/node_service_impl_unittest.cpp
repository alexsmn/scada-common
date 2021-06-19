#include "node_service/v3/node_service_impl.h"

#include "base/test/test_executor.h"
#include "core/attribute_service.h"
#include "core/monitored_item_service_mock.h"
#include "model/node_id_util.h"

#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

namespace v3 {

using namespace testing;

}  // namespace v3
