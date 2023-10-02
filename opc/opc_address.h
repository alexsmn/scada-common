#pragma once

#include <optional>
#include <string>

namespace opc {

struct OpcItemId {
  std::string machine_name;
  std::string prog_id;
  std::string item_id;
};

enum class OpcAddressType { Server, Branch, Item };

struct OpcAddressView {
  // |prog_id| cannot be empty.
  static OpcAddressView MakeServer(std::string_view prog_id);

  // |prog_id| cannot be empty. If empty |item_id| is provided, then
  // the call evolves to `MakeServer()`.
  static OpcAddressView MakeBranch(std::string_view prog_id,
                                   std::string_view item_id);

  // Both |prog_id| and |item_id| cannot be empty.
  static OpcAddressView MakeItem(std::string_view prog_id,
                                 std::string_view item_id);

  // WARNING: This method must be very performant, as a service locator may
  // invoke it for each requested node.
  static std::optional<OpcAddressView> Parse(std::string_view address);

  std::string ToString() const;

  OpcAddressType type = OpcAddressType::Item;
  std::string_view machine_name;
  std::string_view prog_id;
  // Never empty for item nodes.
  std::string_view item_id;
};

}  // namespace opc
