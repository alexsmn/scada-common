#include "opc/opc_address.h"

#include <cassert>

namespace opc {

namespace {
const char kOpcMachineNamePrefix[] = "\\\\";
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

std::string OpcAddressView::ToString() const {
  auto local_string = ToStringWithoutMachineName();
  if (local_string.empty()) {
    return {};
  }

  return machine_name.empty()
             ? local_string
             : std::string{kOpcMachineNamePrefix} + std::string{machine_name} +
                   kOpcPathDelimiter + local_string;
}

std::string OpcAddressView::ToStringWithoutMachineName() const {
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

  std::string_view machine_name;
  if (address.starts_with(kOpcMachineNamePrefix)) {
    address = address.substr(std::strlen(kOpcMachineNamePrefix));
    auto p = address.find(kOpcPathDelimiter);
    if (p == 0 || p == address.npos) {
      return std::nullopt;
    }
    machine_name = address.substr(0, p);
    address = address.substr(p + 1);
  }

  if (address.empty()) {
    return std::nullopt;
  }

  auto p = address.find(kOpcPathDelimiter);

  if (p == 0) {
    return std::nullopt;
  }

  // Server node ID consisting from only a prog ID, like `Prog.ID`.
  if (p == address.npos) {
    return OpcAddressView{.type = OpcAddressType::Server,
                          .machine_name = machine_name,
                          .prog_id = address};
  }

  auto prog_id = address.substr(0, p);
  auto item_id = address.substr(p + 1);

  // Server node ID ending with a delimiter, like `Prog.ID\`.
  if (item_id.empty()) {
    return OpcAddressView{.type = OpcAddressType::Server,
                          .machine_name = machine_name,
                          .prog_id = prog_id};
  }

  // Branch node ID ending with a delimiter, like `Prog.ID\A.B.C\`. So far it
  // includes edge cases like `Prog.ID\\\`.
  if (item_id.back() == kOpcPathDelimiter) {
    return OpcAddressView{.type = OpcAddressType::Branch,
                          .machine_name = machine_name,
                          .prog_id = prog_id,
                          .item_id = item_id.substr(0, item_id.size() - 1)};
  }

  return OpcAddressView{.type = OpcAddressType::Item,
                        .machine_name = machine_name,
                        .prog_id = prog_id,
                        .item_id = item_id};
}

}  // namespace opc
