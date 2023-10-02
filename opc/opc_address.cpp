#include "opc/opc_address.h"

#include <cassert>

namespace opc {

namespace {
const char kOpcPathDelimiter = '\\';
const char kOpcCustomItemDelimiter = '.';
}  // namespace

// static
OpcAddressView OpcAddressView::MakeServer(std::string_view prog_id) {
  assert(!prog_id.empty());
  return {.type = OpcAddressType::Server, .prog_id = prog_id};
}

// |item_id| can be empty for server node ID.
OpcAddressView OpcAddressView::MakeBranch(std::string_view prog_id,
                                          std::string_view item_id) {
  assert(!prog_id.empty());

  if (item_id.empty()) {
    return MakeServer(prog_id);
  }

  return {
      .type = OpcAddressType::Branch, .prog_id = prog_id, .item_id = item_id};
}

OpcAddressView OpcAddressView::MakeItem(std::string_view prog_id,
                                        std::string_view item_id) {
  assert(!prog_id.empty());
  assert(!item_id.empty());

  return {.type = OpcAddressType::Item, .prog_id = prog_id, .item_id = item_id};
}

// static
std::string OpcAddressView::ToString() const {
  // TODO: Support machine name.
  assert(machine_name.empty());
  assert(!prog_id.empty());

  switch (type) {
    case OpcAddressType::Server:
      return std::string{prog_id};
    case OpcAddressType::Branch:
      assert(!item_id.empty());
      return std::string{prog_id} + kOpcPathDelimiter + std::string{item_id} +
             kOpcPathDelimiter;
    case OpcAddressType::Item:
      assert(!item_id.empty());
      return std::string{prog_id} + kOpcPathDelimiter + std::string{item_id};
    default:
      assert(false);
      return {};
  }
}

// static
std::optional<OpcAddressView> OpcAddressView::Parse(std::string_view address) {
  // TODO: Support machine name.

  if (address.empty()) {
    return std::nullopt;
  }

  auto p = address.find(kOpcPathDelimiter);

  if (p == 0) {
    return std::nullopt;
  }

  // Server node ID consisting from only a prog ID, like `Prog.ID`.
  if (p == address.npos) {
    return OpcAddressView{.type = OpcAddressType::Server, .prog_id = address};
  }

  auto prog_id = address.substr(0, p);
  auto item_id = address.substr(p + 1);

  // Server node ID ending with a delimiter, like `Prog.ID\`.
  if (item_id.empty()) {
    return OpcAddressView{.type = OpcAddressType::Server, .prog_id = prog_id};
  }

  // Branch node ID ending with a delimiter, like `Prog.ID\A.B.C\`. So far it
  // includes edge cases like `Prog.ID\\\`.
  if (item_id.back() == kOpcPathDelimiter) {
    return OpcAddressView{.type = OpcAddressType::Branch,
                          .prog_id = prog_id,
                          .item_id = item_id.substr(0, item_id.size() - 1)};
  }

  return OpcAddressView{
      .type = OpcAddressType::Item, .prog_id = prog_id, .item_id = item_id};
}

}  // namespace opc