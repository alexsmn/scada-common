#pragma once

// Hand-written companions to the generated `security_node_ids.h`. The generated
// header carries the plain node-id constants derived from the nodesets; the
// helpers here need logic or reference non-SCADA standard nodes and therefore
// cannot be generated. Include this header (instead of `security_node_ids.h`)
// when you need `UserNodeId` or `RoleSet`.

#include "model/namespaces.h"
#include "model/security_node_ids.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"

namespace scada::security::id {

// The server's RoleSet object. Unlike the SCADA-namespace security nodes this
// is the standard OPC UA Server ServerCapabilities RoleSet in namespace 0.
// OPC UA Part 3 §8.40 RoleSet,
// https://reference.opcfoundation.org/Core/Part3/v105/docs/8.40
constexpr scada::NodeId RoleSet{scada::id::Server_ServerCapabilities_RoleSet,
                                NamespaceIndexes::NS0};

}  // namespace scada::security::id

namespace scada::security {

// Maps a numeric user id (the credential key, e.g. a UserCredentials row or a
// password.dat entry) back to the user's NodeId. The built-in root user is a
// static node in the SCADA namespace; all other users are UserType nodes in
// the USER namespace.
inline scada::NodeId UserNodeId(unsigned user_id) {
  if (user_id == id::RootUser.numeric_id()) {
    return id::RootUser;
  }
  return scada::NodeId{user_id, NamespaceIndexes::USER};
}

}  // namespace scada::security
