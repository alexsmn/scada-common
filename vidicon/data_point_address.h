#pragma once

#include <boost/container_hash/hash.hpp>
#include <optional>
#include <string>

namespace vidicon {

using VidiconObjectId = unsigned;
using VidiconFileId = unsigned;

inline const VidiconObjectId kRootVidiconObjectId = 0;
inline const VidiconFileId kRootVidiconFileId = 0;

struct DataPointAddress {
  std::wstring opc_address;
  VidiconObjectId object_id = 0;

  bool operator==(const DataPointAddress& other) const = default;
};

std::optional<DataPointAddress> ParseDataPointAddress(std::wstring_view str);

}  // namespace vidicon

namespace std {

template <>
struct hash<vidicon::DataPointAddress> {
  std::size_t operator()(
      const vidicon::DataPointAddress& address) const noexcept {
    std::size_t seed = 0;
    boost::hash_combine(seed, address.opc_address);
    boost::hash_combine(seed, address.object_id);
    return seed;
  }
};

}  // namespace std
