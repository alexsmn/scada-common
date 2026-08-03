#include "node_service/node_fetch_status.h"

#include "base/bit_mask_string.h"

std::ostream& operator<<(std::ostream& stream, NodeFetchStatus status) {
  constexpr std::string_view kBitStrings[] = {
      "Node",
      "Children",
      "NonHierInverse",
  };
  return stream << scada::base::BitMaskToString(static_cast<unsigned>(status),
                                                kBitStrings);
}
