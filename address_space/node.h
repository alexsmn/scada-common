#pragma once

#include <memory>

#include "base/ranges.h"
#include "core/configuration_types.h"
#include "core/node_attributes.h"
#include "core/node_class.h"
#include "address_space/node_observer.h"
#include "address_space/reference.h"
#include "core/status.h"
#include "core/variant.h"

namespace scada {

class AttributeService;
class MethodService;
class MonitoredItemService;
class Node;
class ReferenceType;
class TypeDefinition;
class ViewService;
struct Reference;

using References = std::vector<Reference>;

class Node {
 public:
  Node();
  virtual ~Node();

  const NodeId& id() const { return id_; }
  void set_id(NodeId id) { id_ = std::move(id); }

  virtual NodeClass GetNodeClass() const = 0;

  virtual QualifiedName GetBrowseName() const;
  void SetBrowseName(QualifiedName browse_name) {
    browse_name_ = std::move(browse_name);
  }

  virtual LocalizedText GetDisplayName() const;
  void SetDisplayName(LocalizedText display_name) {
    display_name_ = std::move(display_name);
  }

  const References& forward_references() const { return forward_references_; }
  const References& inverse_references() const { return inverse_references_; }

  TypeDefinition* type_definition() { return type_definition_; }
  const TypeDefinition* type_definition() const { return type_definition_; }

  virtual void Startup() {}
  virtual void Shutdown() {}

  virtual void OnNodeCreated() {}
  virtual void OnNodeDeleted() {}
  virtual void OnNodeMoved() {}
  virtual void OnNodeModified(const AttributeSet& attributes,
                              const PropertyIds& property_ids) {}

  virtual void AddReference(const ReferenceType& reference_type,
                            bool forward,
                            Node& node);
  virtual void DeleteReference(const ReferenceType& reference_type,
                               bool forward,
                               Node& node);

  virtual AttributeService* GetAttributeService() { return nullptr; }
  virtual ViewService* GetViewService() { return nullptr; }
  virtual MethodService* GetMethodService() { return nullptr; }
  virtual MonitoredItemService* GetMonitoredItemService() { return nullptr; }

 private:
  NodeId id_;

  QualifiedName browse_name_;
  LocalizedText display_name_;

  References forward_references_;
  References inverse_references_;

  TypeDefinition* type_definition_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Node);
};

void AddReference(const ReferenceType& reference_type,
                  Node& source,
                  Node& target);
void DeleteReference(const ReferenceType& reference_type,
                     Node& source,
                     Node& target);

}  // namespace scada
