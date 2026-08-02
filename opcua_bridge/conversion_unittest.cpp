#include "opcua_bridge/conversion.h"
#include "opcua_bridge/service_conversion.h"

#include "scada/authorization.h"
#include "scada/extension_object.h"
#include "scada/identity_mapping_rule_encoding.h"
#include "scada/range_encoding.h"
#include "scada/user_management_encoding.h"

#include "opcua/events/event_filter.h"
#include "opcua/monitored/monitored_item.h"
#include "opcua/transport/binary/codec_utils.h"

#include <any>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace scada::opcua_bridge {
namespace {

// Round-trips a scada value through opcua and back; the result must equal the
// original. This simultaneously exercises both conversion directions and
// proves the two type universes coexist in one translation unit.
template <class T>
void ExpectRoundTrip(const T& original) {
  EXPECT_EQ(ToScada(ToOpcua(original)), original);
}

TEST(ConversionTest, NodeId) {
  ExpectRoundTrip(scada::NodeId{42u});
  ExpectRoundTrip(scada::NodeId{1234u, 3});
  ExpectRoundTrip(scada::NodeId{scada::String{"the.node"}, 2});
  ExpectRoundTrip(scada::NodeId{scada::ByteString{'a', 'b', 'c'}, 1});
  ExpectRoundTrip(scada::NodeId{});
}

TEST(ConversionTest, RolePermissionTypeExtensionObjectEncodesBinaryBody) {
  const scada::RolePermissionType role{
      .role_id = scada::NodeId{15680u, 0},  // WellKnownRole_Operator
      .permissions = scada::Permission::kRead | scada::Permission::kWrite};
  const scada::ExtensionObject scada_object{
      scada::ExpandedNodeId{scada::NodeId{96u, 0}}, std::any{role}};

  const opcua::ExtensionObject opcua_object = ToOpcua(scada_object);

  // The wire encoding id is RolePermissionType_Encoding_DefaultBinary (i=128).
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(), 128u);

  // The body decodes back to the original role id and permission bits.
  const auto* body = std::any_cast<std::vector<char>>(&opcua_object.value());
  ASSERT_NE(body, nullptr);
  opcua::binary::Decoder decoder{*body};
  opcua::NodeId decoded_role_id;
  std::uint32_t decoded_permissions = 0;
  ASSERT_TRUE(decoder.Decode(decoded_role_id));
  ASSERT_TRUE(decoder.Decode(decoded_permissions));
  EXPECT_EQ(decoded_role_id.numeric_id(), 15680u);
  EXPECT_EQ(decoded_permissions,
            static_cast<std::uint32_t>(scada::Permission::kRead |
                                       scada::Permission::kWrite));
}

// An IdentityMappingRuleType payload survives the boundary in BOTH directions:
// the encode side emits the DefaultBinary body (CriteriaType Int32 + Criteria
// String per the official 1.05 Opc.Ua.Types.bsd.xml), and the decode side
// reconstructs the typed rule from that body — so AddIdentity/RemoveIdentity
// method arguments (OPC UA Part 18 §4.4.5/§4.4.6) arrive structured.
TEST(ConversionTest, IdentityMappingRuleExtensionObjectRoundTrips) {
  const scada::IdentityMappingRule rule{
      .criteria_type = scada::IdentityCriteriaType::kUserName,
      .criteria = "svc-site-a"};
  const scada::ExtensionObject scada_object =
      scada::MakeIdentityMappingRuleObject(rule);

  const opcua::ExtensionObject opcua_object = ToOpcua(scada_object);

  // The wire encoding id is IdentityMappingRuleType_Encoding_DefaultBinary.
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(),
            scada::kIdentityMappingRuleTypeDefaultBinaryId);

  const scada::ExtensionObject decoded = ToScada(opcua_object);
  EXPECT_EQ(decoded.data_type_id().node_id().numeric_id(),
            scada::kIdentityMappingRuleTypeDataTypeId);
  const auto decoded_rule = scada::DecodeIdentityMappingRule(decoded);
  ASSERT_TRUE(decoded_rule.has_value());
  EXPECT_EQ(*decoded_rule, rule);
}

// A UserManagementDataType payload survives the boundary in BOTH directions:
// the encode side emits the DefaultBinary body (UserName String +
// UserConfiguration UInt32 + Description String, in the field order of the
// official 1.05 Opc.Ua.Types.bsd.xml) and the decode side reconstructs the
// typed record — so an OPC UA client reading the UserManagement object's Users
// property (OPC UA Part 18 §5.2.2) gets structured accounts, not opaque bytes.
TEST(ConversionTest, UserManagementUserExtensionObjectRoundTrips) {
  const scada::UserManagementDataType user{
      .user_name = "ivanov",
      .user_configuration = scada::UserConfiguration::kDisabled,
      .description = "Dispatcher, north"};
  const scada::ExtensionObject scada_object =
      scada::MakeUserManagementObject(user);

  const opcua::ExtensionObject opcua_object = ToOpcua(scada_object);

  // The wire encoding id is UserManagementDataType_Encoding_DefaultBinary.
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(),
            scada::kUserManagementDataTypeDefaultBinaryId);

  const scada::ExtensionObject decoded = ToScada(opcua_object);
  EXPECT_EQ(decoded.data_type_id().node_id().numeric_id(),
            scada::kUserManagementDataTypeId);
  const auto decoded_user = scada::DecodeUserManagementObject(decoded);
  ASSERT_TRUE(decoded_user.has_value());
  EXPECT_EQ(*decoded_user, user);
}

// The description is the one free-text field an operator types, so it must
// survive non-ASCII content: the body encodes it as a UA String (UTF-8), and a
// byte-length mistake here would truncate or corrupt every following field.
TEST(ConversionTest, UserManagementUserCarriesNonAsciiDescription) {
  const scada::UserManagementDataType user{.user_name = "ivanov",
                                           .description = "Диспетчер, север"};

  const auto decoded = scada::DecodeUserManagementObject(
      ToScada(ToOpcua(scada::MakeUserManagementObject(user))));

  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->description, user.description);
  EXPECT_EQ(decoded->user_name, user.user_name);
}

// The UserManagement object's PasswordLength is a Range (OPC UA Part 18
// §5.2.2), so a client can only apply the server's password policy if the
// low/high pair survives the boundary.
TEST(ConversionTest, RangeExtensionObjectRoundTrips) {
  const scada::Range range{.low = 8, .high = 64};

  const opcua::ExtensionObject opcua_object =
      ToOpcua(scada::MakeRangeObject(range));
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(),
            scada::kRangeDefaultBinaryId);

  const auto decoded = scada::DecodeRangeObject(ToScada(opcua_object));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, range);
}

TEST(ConversionTest, QualifiedName) {
  ExpectRoundTrip(scada::QualifiedName{"Temperature", 4});
  ExpectRoundTrip(scada::QualifiedName{});
}

TEST(ConversionTest, ExpandedNodeId) {
  ExpectRoundTrip(scada::ExpandedNodeId{scada::NodeId{7u, 1}});
}

TEST(ConversionTest, StatusAndCode) {
  EXPECT_EQ(ToScada(ToOpcua(scada::StatusCode::Bad_WrongNodeId)),
            scada::StatusCode::Bad_WrongNodeId);
  ExpectRoundTrip(scada::Status{scada::StatusCode::Good});
  ExpectRoundTrip(scada::Status{scada::StatusCode::Bad_WrongLoginCredentials});
  ExpectRoundTrip(scada::Status{scada::StatusCode::Bad_LicenseExpired});
}

// The boundary translates SCADA-internal status codes to the standard OPC UA
// StatusCode values that opcuapp puts on the wire (Opc.Ua.StatusCodes.csv).
TEST(ConversionTest, StatusCodeMapsToStandardOpcUaWireValue) {
  const auto wire = [](scada::StatusCode code) {
    return opcua::Status{ToOpcua(code)}.full_code();
  };
  EXPECT_EQ(wire(scada::StatusCode::Bad_NothingToDo), 0x800F0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_TooManyOperations), 0x80100000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongNodeId),
            0x80340000u);  // NodeIdUnknown
  EXPECT_EQ(wire(scada::StatusCode::Bad_ViewIdUnknown), 0x806B0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_TimestampsToReturnInvalid),
            0x802B0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_NoSubscription), 0x80790000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_HistoryOperationInvalid), 0x80710000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_UserAccessDenied),
            0x801F0000u);  // BadUserAccessDenied
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongLoginCredentials),
            0x80210000u);  // BadIdentityTokenRejected
  EXPECT_EQ(wire(scada::StatusCode::Bad_NotSupported),
            0x803D0000u);  // BadNotSupported
  EXPECT_EQ(wire(scada::StatusCode::Bad_LicenseExpired),
            0x810E0000u);  // BadLicenseExpired
  EXPECT_EQ(wire(scada::StatusCode::Bad_WaitingForInitialData),
            0x80320000u);  // BadWaitingForInitialData
  EXPECT_EQ(wire(scada::StatusCode::Bad_Disconnected),
            0x80310000u);  // BadNoCommunication
  EXPECT_EQ(wire(scada::StatusCode::Bad_SessionForcedLogoff),
            0x80260000u);  // BadSessionClosed
  EXPECT_EQ(wire(scada::StatusCode::Bad_DuplicateNodeId),
            0x805E0000u);  // BadNodeIdExists
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongParentId),
            0x805B0000u);  // BadParentNodeIdInvalid
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongTypeId),
            0x80630000u);  // BadTypeDefinitionInvalid
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongNodeClass),
            0x805F0000u);  // BadNodeClassInvalid
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongReferenceId),
            0x804C0000u);  // BadReferenceTypeIdInvalid
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongTargetId),
            0x80650000u);  // BadTargetNodeIdInvalid
  EXPECT_EQ(wire(scada::StatusCode::Bad_CantParseString),
            0x80740000u);  // BadTypeMismatch
  // Both core codes for "the Server will not store this value" go out as the
  // one standard code — OPC UA Part 4 §7.39 Bad_OutOfRange.
  EXPECT_EQ(wire(scada::StatusCode::Bad_TooLongString),
            0x803C0000u);  // BadOutOfRange
  EXPECT_EQ(wire(scada::StatusCode::Bad_OutOfRange),
            0x803C0000u);  // BadOutOfRange
  EXPECT_EQ(wire(scada::StatusCode::Bad_UnsupportedProtocolVersion),
            0x80BE0000u);  // BadProtocolVersionUnsupported
  // Good is unchanged.
  EXPECT_EQ(wire(scada::StatusCode::Good), 0x00000000u);
}

// The codes `Qualifier::ToStatus()` produces now reach standard OPC UA clients
// as the DataValue StatusCode, so their wire values are pinned here.
//
// None of the SCADA quality subcodes is defined by the standard (verified
// against `opcua/ua/ua_status_codes.h`, generated from the OPC Foundation
// table): a conforming client therefore falls back to the severity bits rather
// than misreading them as an unrelated standard code — the same rule the Bad
// codes follow via the 0x2000+ vendor range. Severity is the load-bearing part
// and is asserted explicitly.
TEST(ConversionTest, QualityProjectionWireValues) {
  const auto wire = [](scada::StatusCode code) {
    return opcua::Status{ToOpcua(code)}.full_code();
  };
  const auto severity = [](scada::StatusCode code) {
    return static_cast<int>(opcua::GetSeverity(ToOpcua(code)));
  };
  constexpr int kGood = static_cast<int>(opcua::StatusSeverity::Good);
  constexpr int kUncertain = static_cast<int>(opcua::StatusSeverity::Uncertain);
  constexpr int kBad = static_cast<int>(opcua::StatusSeverity::Bad);

  EXPECT_EQ(wire(scada::StatusCode::Uncertain_DeviceFlag), 0x40010000u);
  EXPECT_EQ(severity(scada::StatusCode::Uncertain_DeviceFlag), kUncertain);
  EXPECT_EQ(wire(scada::StatusCode::Uncertain_Misconfigured), 0x40020000u);
  EXPECT_EQ(severity(scada::StatusCode::Uncertain_Misconfigured), kUncertain);
  EXPECT_EQ(wire(scada::StatusCode::Uncertain_Disconnected), 0x40030000u);
  EXPECT_EQ(severity(scada::StatusCode::Uncertain_Disconnected), kUncertain);
  EXPECT_EQ(wire(scada::StatusCode::Uncertain_NotUpdated), 0x40040000u);
  EXPECT_EQ(severity(scada::StatusCode::Uncertain_NotUpdated), kUncertain);

  EXPECT_EQ(wire(scada::StatusCode::Good_Sporadic), 0x00020000u);
  EXPECT_EQ(severity(scada::StatusCode::Good_Sporadic), kGood);
  EXPECT_EQ(wire(scada::StatusCode::Good_Backup), 0x00030000u);
  EXPECT_EQ(severity(scada::StatusCode::Good_Backup), kGood);
  EXPECT_EQ(wire(scada::StatusCode::Good_Manual), 0x00040000u);
  EXPECT_EQ(severity(scada::StatusCode::Good_Manual), kGood);
  EXPECT_EQ(wire(scada::StatusCode::Good_Simulated), 0x00050000u);
  EXPECT_EQ(severity(scada::StatusCode::Good_Simulated), kGood);

  // A FAILED qualifier projects onto plain Bad, which IS the standard `Bad`.
  EXPECT_EQ(wire(scada::StatusCode::Bad), 0x80000000u);
  EXPECT_EQ(severity(scada::StatusCode::Bad), kBad);
}

// Every core status code must survive the OPC UA wire crossing losslessly
// (tier-to-tier links carry device and quality codes through opcuapp), and no
// Bad code may leak a core-internal value that collides with an unrelated
// standard code: whatever goes on the wire is either the matching standard
// code or a vendor-extension SubCode (>= 0x2000) that the standard leaves
// undefined.
TEST(ConversionTest, AllStatusCodesRoundTripAndAvoidStandardCollisions) {
  using scada::StatusCode;
  constexpr StatusCode kAllCodes[] = {
      StatusCode::Good,
      StatusCode::Good_Pending,
      StatusCode::Good_Sporadic,
      StatusCode::Good_Backup,
      StatusCode::Good_Manual,
      StatusCode::Good_Simulated,
      StatusCode::Uncertain,
      StatusCode::Uncertain_DeviceFlag,
      StatusCode::Uncertain_Misconfigured,
      StatusCode::Uncertain_Disconnected,
      StatusCode::Uncertain_NotUpdated,
      StatusCode::Uncertain_StateWasNotChanged,
      StatusCode::Bad,
      StatusCode::Bad_WrongLoginCredentials,
      StatusCode::Bad_UserIsAlreadyLoggedOn,
      StatusCode::Bad_UnsupportedProtocolVersion,
      StatusCode::Bad_ObjectIsBusy,
      StatusCode::Bad_WrongNodeId,
      StatusCode::Bad_WrongDeviceId,
      StatusCode::Bad_Disconnected,
      StatusCode::Bad_SessionForcedLogoff,
      StatusCode::Bad_Timeout,
      StatusCode::Bad_CantDeleteDependentNode,
      StatusCode::Bad_ServerWasShutDown,
      StatusCode::Bad_WrongMethodId,
      StatusCode::Bad_CantDeleteOwnUser,
      StatusCode::Bad_DuplicateNodeId,
      StatusCode::Bad_UnsupportedFileVersion,
      StatusCode::Bad_WrongTypeId,
      StatusCode::Bad_WrongParentId,
      StatusCode::Bad_SessionIsLoggedOff,
      StatusCode::Bad_WrongSubscriptionId,
      StatusCode::Bad_WrongIndex,
      StatusCode::Bad_Iec60870UnknownType,
      StatusCode::Bad_Iec60870UnknownCot,
      StatusCode::Bad_Iec60870UnknownDevice,
      StatusCode::Bad_Iec60870UnknownAddress,
      StatusCode::Bad_Iec60870UnknownError,
      StatusCode::Bad_WrongCallArguments,
      StatusCode::Bad_CantParseString,
      StatusCode::Bad_OutOfRange,
      StatusCode::Bad_WrongPropertyId,
      StatusCode::Bad_WrongReferenceId,
      StatusCode::Bad_WrongNodeClass,
      StatusCode::Bad_WrongAttributeId,
      StatusCode::Bad_Iec61850Error,
      StatusCode::Bad_NothingToDo,
      StatusCode::Bad_BrowseNameInvalid,
      StatusCode::Bad_WrongTargetId,
      StatusCode::Bad_MonitoredItemIdInvalid,
      StatusCode::Bad_MessageNotAvailable,
      StatusCode::Bad_ApplicationSignatureInvalid,
      StatusCode::Bad_TooManyOperations,
      StatusCode::Bad_TooManyMonitoredItems,
      StatusCode::Bad_SequenceNumberUnknown,
      StatusCode::Bad_NoContinuationPoints,
      StatusCode::Bad_TimestampsToReturnInvalid,
      StatusCode::Bad_ViewIdUnknown,
      StatusCode::Bad_HistoryOperationInvalid,
      StatusCode::Bad_NoSubscription,
      StatusCode::Bad_UserAccessDenied,
      StatusCode::Bad_NotSupported,
      StatusCode::Bad_LicenseExpired,
      StatusCode::Bad_WaitingForInitialData,
  };
  // Core codes that deliberately collapse onto a wire code another core code
  // owns (SCADA_OPCUA_STATUS_CODE_MAP_TO_WIRE). They cannot round-trip — that
  // is the point of the second table — so what is asserted is the half that
  // matters for conformance: they reach the wire as the standard code rather
  // than leaking their internal enumerator value through the default cast.
  EXPECT_EQ(ToOpcua(StatusCode::Bad_TooLongString),
            opcua::StatusCode::Bad_OutOfRange);
  EXPECT_EQ(ToScada(opcua::StatusCode::Bad_OutOfRange),
            StatusCode::Bad_OutOfRange);

  for (const auto code : kAllCodes) {
    EXPECT_EQ(ToScada(ToOpcua(code)), code) << ToString(code);
    // Severity must survive the mapping.
    EXPECT_EQ(static_cast<int>(scada::GetSeverity(code)),
              static_cast<int>(opcua::GetSeverity(ToOpcua(code))))
        << ToString(code);
  }
  // The core-internal sequential values (SubCode < 0x100 in the Bad range but
  // not equal to a deliberate standard mapping) must never appear on the wire
  // for codes the bridge knows: the wire SubCode is either a standard value
  // (< 0x2000, asserted per-code above and in
  // StatusCodeMapsToStandardOpcUaWireValue) or in the vendor-extension range.
  for (const auto code :
       {StatusCode::Bad_UserIsAlreadyLoggedOn, StatusCode::Bad_WrongDeviceId,
        StatusCode::Bad_CantDeleteOwnUser, StatusCode::Bad_WrongPropertyId,
        StatusCode::Bad_Iec60870UnknownType, StatusCode::Bad_Iec60870UnknownCot,
        StatusCode::Bad_Iec60870UnknownDevice,
        StatusCode::Bad_Iec60870UnknownAddress,
        StatusCode::Bad_Iec60870UnknownError, StatusCode::Bad_Iec61850Error}) {
    const unsigned sub_code =
        (opcua::Status{ToOpcua(code)}.full_code() >> 16) & 0x3FFF;
    EXPECT_GE(sub_code, 0x2000u) << ToString(code);
  }
}

TEST(ConversionTest, Time) {
  const auto scada_time = base::DecodeWireTime(123456789);
  EXPECT_EQ(ToOpcua(scada_time).ToInternalValue(), 1234567890);
  ExpectRoundTrip(scada_time);
  ExpectRoundTrip(scada::Time{});
  ExpectRoundTrip(scada::kMinTime);
  ExpectRoundTrip(scada::kMaxTime);
}

TEST(ConversionTest, Duration) {
  const auto scada_duration = std::chrono::microseconds(1250);
  EXPECT_DOUBLE_EQ(ToOpcua(scada_duration).ToInternalValue(), 1.25);
  ExpectRoundTrip(scada_duration);
  ExpectRoundTrip(scada::Duration::min());
  ExpectRoundTrip(scada::Duration::max());
}

TEST(ConversionTest, VariantScalars) {
  ExpectRoundTrip(scada::Variant{true});
  ExpectRoundTrip(scada::Variant{scada::Int32{-7}});
  ExpectRoundTrip(scada::Variant{scada::UInt64{99}});
  ExpectRoundTrip(scada::Variant{scada::Double{3.5}});
  ExpectRoundTrip(scada::Variant{scada::String{"hello"}});
  ExpectRoundTrip(scada::Variant{scada::NodeId{55u, 2}});
  ExpectRoundTrip(scada::Variant{scada::QualifiedName{"q", 1}});
  ExpectRoundTrip(scada::Variant{});
}

TEST(ConversionTest, VariantArrays) {
  ExpectRoundTrip(scada::Variant{std::vector<scada::Int32>{1, 2, 3}});
  ExpectRoundTrip(scada::Variant{std::vector<scada::String>{"a", "b"}});
  ExpectRoundTrip(scada::Variant{
      std::vector<scada::NodeId>{scada::NodeId{1u}, scada::NodeId{2u}}});
}

TEST(ConversionTest, ReadValueId) {
  ExpectRoundTrip(
      scada::ReadValueId{.node_id = scada::NodeId{9u, 1},
                         .attribute_id = scada::AttributeId::DisplayName});
}

TEST(ConversionTest, BrowseDescriptionAndResult) {
  ExpectRoundTrip(
      scada::BrowseDescription{.node_id = scada::NodeId{84u},
                               .direction = scada::BrowseDirection::Forward,
                               .reference_type_id = scada::NodeId{33u},
                               .include_subtypes = true});

  scada::BrowseResult br;
  br.status_code = scada::StatusCode::Good;
  br.references.push_back(
      scada::ReferenceDescription{.reference_type_id = scada::NodeId{35u},
                                  .forward = true,
                                  .node_id = scada::NodeId{2253u},
                                  .node_class = scada::NodeClass::Object});
  ExpectRoundTrip(br);
}

TEST(ConversionTest, DataValue) {
  scada::DataValue dv;
  dv.value = scada::Variant{scada::Double{42.0}};
  dv.status_code = scada::StatusCode::Good;
  dv.source_timestamp = base::DecodeWireTime(1000);
  dv.server_timestamp = base::DecodeWireTime(2000);
  ExpectRoundTrip(dv);
}

// The wire MonitoringFilter has no typed event/aggregate slot, so the bridge
// serializes core event/aggregate filters to a self-describing json blob and
// back. These round-trips guard that path (the typed filter must survive).
TEST(ConversionTest, MonitoringFilterEventRoundTrip) {
  scada::EventFilter ef;
  ef.types = scada::EventFilter::ACKED | scada::EventFilter::UNACKED;
  ef.of_type = {scada::NodeId{2041u},
                scada::NodeId{scada::String{"AlarmType"}, 1}};
  ef.child_of = {scada::NodeId{85u}};
  scada::MonitoringParameters params;
  params.filter = ef;

  const scada::MonitoringFilter round = ToScada(ToOpcua(params)).filter;
  ASSERT_TRUE(std::holds_alternative<scada::EventFilter>(round));
  EXPECT_EQ(std::get<scada::EventFilter>(round), ef);
}

TEST(ConversionTest, MonitoringFilterAggregateRoundTrip) {
  scada::AggregateFilter af;
  af.start_time = base::DecodeWireTime(123456789);
  af.interval = std::chrono::milliseconds(500);
  af.aggregate_type = scada::NodeId{2342u};
  scada::MonitoringParameters params;
  params.filter = af;

  const scada::MonitoringFilter round = ToScada(ToOpcua(params)).filter;
  ASSERT_TRUE(std::holds_alternative<scada::AggregateFilter>(round));
  EXPECT_EQ(std::get<scada::AggregateFilter>(round), af);
}

// A monitored item's initial status report carries no event. It must stay
// distinguishable from a real event across the wire: reassembling it into a
// default-constructed event yields event_id 0, which is not a legal event
// (OPC UA Part 5 §6.4.2 BaseEventType requires a non-null EventId,
// https://reference.opcfoundation.org/Core/Part5/v105/docs/6.4.2) and which
// the aggregating proxy used to relay onward until its gRPC projection
// panicked.
TEST(ConversionTest, EventFieldListAllNullFieldsCarriesNoEvent) {
  opcua::EventFieldList list;
  list.client_handle = 7;
  list.event_fields.resize(opcua::DefaultEventFieldPaths().size());

  const scada::MonitoredItemNotification notification =
      ToScada(opcua::ItemNotification{list});

  const auto* event = std::get_if<scada::EventNotification>(&notification);
  ASSERT_NE(event, nullptr);
  EXPECT_EQ(event->client_handle, 7u);
  EXPECT_FALSE(event->event.has_value());
}

TEST(ConversionTest, EventFieldListEmptyCarriesNoEvent) {
  opcua::EventFieldList list;
  list.client_handle = 7;

  const scada::MonitoredItemNotification notification =
      ToScada(opcua::ItemNotification{list});

  const auto* event = std::get_if<scada::EventNotification>(&notification);
  ASSERT_NE(event, nullptr);
  EXPECT_FALSE(event->event.has_value());
}

// The guard above must not cost fidelity for a real event: the default
// full-fidelity projection still reassembles with its identity intact.
TEST(ConversionTest, EventFieldListDefaultProjectionKeepsEventId) {
  opcua::Event source;
  source.event_id = 0x0123456789abcdefull;
  source.event_type_id = opcua::NodeId{opcua::id::SystemEventType};
  source.time = opcua::DateTime::Now();
  source.receive_time = source.time;
  source.severity = 500;
  source.message = opcua::LocalizedText{u"pump tripped"};

  opcua::EventFieldList list;
  list.client_handle = 9;
  list.event_fields = opcua::ProjectEventFields(opcua::DefaultEventFieldPaths(),
                                                std::any{source});

  const scada::MonitoredItemNotification notification =
      ToScada(opcua::ItemNotification{list});

  const auto* notified = std::get_if<scada::EventNotification>(&notification);
  ASSERT_NE(notified, nullptr);
  const auto* event = std::any_cast<scada::Event>(&notified->event);
  ASSERT_NE(event, nullptr);
  EXPECT_EQ(event->event_id, source.event_id);
  EXPECT_EQ(event->severity, source.severity);
}

}  // namespace
}  // namespace scada::opcua_bridge
