// scada.address_space — named C++20 module facade over the
// common/address_space headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). `export import scada.core;` mirrors
// address_space's PUBLIC link on scada_core.
//
// property-inl.h is an implementation detail included by property.h itself;
// mocks are excluded as everywhere.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "address_space/address_space.h"
#include "address_space/address_space_impl.h"
#include "address_space/address_space_impl2.h"
#include "address_space/address_space_type_system.h"
#include "address_space/address_space_util.h"
#include "address_space/address_space_xml.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/data_variable.h"
#include "address_space/fallback_node_factory.h"
#include "address_space/folder.h"
#include "address_space/generic_node_factory.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/method.h"
#include "address_space/method_service_impl.h"
#include "address_space/mutable_address_space.h"
#include "address_space/node.h"
#include "address_space/node_builder.h"
#include "address_space/node_builder_impl.h"
#include "address_space/node_factory.h"
#include "address_space/node_factory_util.h"
#include "address_space/node_format.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/object.h"
#include "address_space/property.h"
#include "address_space/property_ids.h"
#include "address_space/reference.h"
#include "address_space/standard_address_space.h"
#include "address_space/standard_type_system.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "address_space/view_service_impl.h"

export module scada.address_space;

export import scada.core;

export namespace scada {

// (moved)
using scada::NodeBuilder;

// address_space.h / address_space_impl.h / mutable_address_space.h
using scada::AddressSpace;

// node.h / reference.h / object.h / variable.h / method.h /
// type_definition.h / folder.h / property.h / property_ids.h
using scada::BaseObject;
using scada::BaseVariable;
using scada::ComponentObject;
using scada::DataType;
using scada::DataVariable;
using scada::Folder;
using scada::GenericMethod;
using scada::GenericObject;
using scada::GenericVariable;
using scada::Method;
using scada::Node;
using scada::Object;
using scada::ObjectType;
using scada::Property;
using scada::PropertyDecl;
using scada::PropertyIds;
using scada::Reference;
using scada::References;
using scada::ReferenceType;
using scada::TypeDefinition;
using scada::Variable;
using scada::VariableType;

// local_*.h / *_service_impl.h
using scada::LocalHistoryService;
using scada::LocalMethodService;
using scada::LocalMonitoredItemService;
using scada::LocalNodeManagementService;
using scada::LocalSessionService;

// node_variable_handle.h
using scada::NodeVariableHandle;

// address_space_util.h / node_utils.h / node_format.h
using scada::AsDataType;
using scada::AsObject;
using scada::AsObjectType;
using scada::AsReferenceType;
using scada::AsTypeDefinition;
using scada::AsVariable;
using scada::AsVariableType;
using scada::DataValueFieldId;
using scada::DeleteAllReferences;
using scada::FormatAnalogValue;
using scada::FormatDiscreteValue;
using scada::FormatUnknownValue;
using scada::FormatValue;
using scada::GetBrowseName;
using scada::GetDisplayName;
using scada::GetFullDisplayName;
using scada::GetNonPropReferences;
using scada::GetTsFormat;
using scada::IsDisabled;
using scada::IsNonPropReference;
using scada::IsRefSubtypeOf;
using scada::IsSimulated;
using scada::MakeNodeStates;
using scada::MakeStandardNodeStates;
using scada::ParseDataValueFieldString;
using scada::RefNode;

// address_space_xml.h
using scada::LoadAddressSpaceXml;
using scada::LoadStaticAddressSpace;
using scada::SaveAddressSpaceXml;

}  // namespace scada

export {
  // (moved: global-namespace names)
  using ::AddressSpaceImpl;
  using ::AttributeServiceImpl;
  using ::AttributeServiceImplContext;
  using ::GenericNodeFactory;
  using ::MethodServiceImpl;
  using ::MethodServiceImplContext;
  using ::MutableAddressSpace;
  using ::SyncAttributeServiceImpl;
  using ::SyncViewServiceImpl;
  using ::ViewServiceImpl;
  using ::ViewServiceImplContext;
  // address_space_impl2.h / standard_address_space.h /
  // address_space_type_system.h / standard_type_system.h
  using ::AddressSpaceImpl2;
  using ::AddressSpaceTypeSystem;
  using ::GenericDataVariable;
  using ::GenericProperty;
  using ::StandardAddressSpace;
  using ::StandardTypeSystem;

  // node_builder.h / node_builder_impl.h / node_factory.h /
  // fallback_node_factory.h / node_factory_util.h
  using ::CreateDataVariables;
  using ::CreateMissingProperties;
  using ::FallbackNodeFactory;
  using ::NodeBuilderImpl;
  using ::NodeFactory;
}  // export
