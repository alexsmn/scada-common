#include "attribute_service_impl.h"
#include "base/check.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/range_util.h"
#include "scada/authorization.h"
#include "scada/role_permission_encoding.h"

#include <ranges>

SyncAttributeServiceImpl::SyncAttributeServiceImpl(
    AttributeServiceImplContext&& context)
    : AttributeServiceImplContext{std::move(context)} {}

AttributeServiceImpl::AttributeServiceImpl(
    SyncAttributeService& sync_attribute_service)
    : sync_attribute_service_{sync_attribute_service} {}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
AttributeServiceImpl::Read(scada::ServiceContext context,
                           std::vector<scada::ReadValueId> inputs) {
  auto results = sync_attribute_service_.Read(context, inputs);
  base::Check(results.size() == inputs.size());

  co_return results;
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
AttributeServiceImpl::Write(scada::ServiceContext context,
                            std::vector<scada::WriteValue> inputs) {
  auto results = sync_attribute_service_.Write(context, inputs);
  co_return results;
}

std::vector<scada::DataValue> SyncAttributeServiceImpl::Read(
    const scada::ServiceContext& context,
    std::span<const scada::ReadValueId> inputs) {
  return inputs |
         std::views::transform(
             [this, &context](const scada::ReadValueId& input) {
               return Read(context, input);
             }) |
         to_vector;
}

scada::DataValue SyncAttributeServiceImpl::Read(
    const scada::ServiceContext& context,
    const scada::ReadValueId& input) {
  std::string_view nested_name;
  auto* node = scada::GetNestedNode(address_space_, input.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  if (nested_name.empty())
    return ReadNode(context, *node, input.attribute_id);

  return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};
}

std::vector<scada::StatusCode> SyncAttributeServiceImpl::Write(
    const scada::ServiceContext& context,
    std::span<const scada::WriteValue> inputs) {
  return std::vector<scada::StatusCode>(inputs.size(),
                                        scada::StatusCode::Bad_WrongNodeId);
}

scada::DataValue SyncAttributeServiceImpl::ReadNode(
    const scada::ServiceContext& context,
    const scada::Node& node,
    scada::AttributeId attribute_id) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      return scada::MakeReadResult(static_cast<int>(node.GetNodeClass()));

    case scada::AttributeId::BrowseName:
      base::Check(!node.GetBrowseName().empty());
      return scada::MakeReadResult(node.GetBrowseName());

    case scada::AttributeId::DisplayName:
      return scada::MakeReadResult(node.GetDisplayName());

    // UserRolePermissions: the caller's own effective role permissions, always
    // readable by the session (OPC UA Part 3 §5.2.10). Narrowed from the node's
    // RolePermissions override when it carries one, else from the defaults.
    case scada::AttributeId::UserRolePermissions:
      return scada::MakeReadResult(scada::EncodeRolePermissions(
          node.role_permissions()
              ? scada::UserRolePermissionsFrom(*node.role_permissions(),
                                               context.user_rights(),
                                               context.is_anonymous())
              : scada::UserRolePermissions(context.user_rights(),
                                           context.is_anonymous())));

    // RolePermissions: the node's full role/permission map, readable only with
    // the ReadRolePermissions permission (OPC UA Part 3 §5.2.9). A node may
    // carry a per-node override; otherwise it publishes the server defaults.
    case scada::AttributeId::RolePermissions:
      if (!scada::IsPermitted(context.user_rights(), context.is_anonymous(),
                              scada::Permission::kReadRolePermissions)) {
        return {scada::StatusCode::Bad_UserAccessDenied,
                scada::DateTime::Now()};
      }
      return scada::MakeReadResult(scada::EncodeRolePermissions(
          node.role_permissions() ? *node.role_permissions()
                                   : scada::DefaultRolePermissions()));
  }

  if (auto* variable = scada::AsVariable(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable->GetDataType().id());
      case scada::AttributeId::Value:
        return variable->GetValue();
      // Mandatory VariableNode attributes (OPC UA Part 3 §5.6.2,
      // https://reference.opcfoundation.org/Core/Part3/v105/docs/5.6.2). The
      // node model does not carry these, so return conformant defaults: a
      // scalar ValueRank, current-read access (these static nodes are not
      // writable through this service), no historizing, and no minimum sampling
      // interval. Clients (and the CTT) require these to be readable rather
      // than Bad_AttributeIdInvalid.
      case scada::AttributeId::ValueRank:
        return scada::MakeReadResult(scada::Int32{-1});  // Scalar
      case scada::AttributeId::AccessLevel:
        return scada::MakeReadResult(
            scada::UInt8{scada::access_level::kCurrentRead});
      // UserAccessLevel narrows AccessLevel by the caller's permissions
      // (OPC UA Part 3 §5.6.2). The caller's rights ride on the ServiceContext.
      case scada::AttributeId::UserAccessLevel:
        return scada::MakeReadResult(scada::UInt8{scada::UserAccessLevel(
            scada::access_level::kCurrentRead, context.user_rights(),
            context.is_anonymous())});
      case scada::AttributeId::Historizing:
        return scada::MakeReadResult(false);
      case scada::AttributeId::MinimumSamplingInterval:
        return scada::MakeReadResult(0.0);
    }
  }

  if (auto* variable_type = scada::AsVariableType(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable_type->data_type().id());
      case scada::AttributeId::Value:
        return scada::MakeReadResult(variable_type->default_value());
    }
  }

  if (auto* reference_type = scada::AsReferenceType(&node)) {
    switch (attribute_id) {
      // InverseName is required for non-symmetric ReferenceTypes (OPC UA
      // Part 3 §5.3.2,
      // https://reference.opcfoundation.org/Core/Part3/v105/docs/5.3.2); a
      // symmetric type carries none and reads as Bad_WrongAttributeId.
      case scada::AttributeId::InverseName:
        if (reference_type->inverse_name().empty())
          break;
        return scada::MakeReadResult(reference_type->inverse_name());
    }
  }

  if (node.GetNodeClass() == scada::NodeClass::Method) {
    switch (attribute_id) {
      // A method node is executable; UserExecutable narrows it to callers that
      // hold the Call permission (OPC UA Part 3 §5.6.2, UserExecutable is the
      // user-specific value of Executable).
      case scada::AttributeId::Executable:
        return scada::MakeReadResult(true);
      case scada::AttributeId::UserExecutable:
        return scada::MakeReadResult(
            scada::IsPermitted(context.user_rights(), context.is_anonymous(),
                               scada::Permission::kCall));
    }
  }

  return {scada::StatusCode::Bad_WrongAttributeId, scada::DateTime::Now()};
}
