#pragma once

#include "core/standard_node_ids.h"
#include "common/namespaces.h"

namespace numeric_id {

const scada::NumericId TsFormats = 27;
const scada::NumericId Users = 29;
const scada::NumericId SimulationSignals = 28;
const scada::NumericId HistoricalDatabases = 12;

const scada::NumericId kIec60870DeviceGroup1PollPeriodPropTypeId = 200;

} // numeric_id

const scada::NodeId kServerParamTypeId{128, NamespaceIndexes::SCADA};

const scada::NodeId kObjectWriteManualCommandId{230, NamespaceIndexes::SCADA};

const scada::NodeId kNodeIdPropTypeId{238, NamespaceIndexes::SCADA};
const scada::NodeId kObjectSeverityPropTypeId{144, NamespaceIndexes::SCADA};
const scada::NodeId kObjectInput1PropTypeId{145, NamespaceIndexes::SCADA};
const scada::NodeId kObjectInput2PropTypeId{146, NamespaceIndexes::SCADA};
const scada::NodeId kObjectOutputPropTypeId{147, NamespaceIndexes::SCADA};
const scada::NodeId kObjectOutputConditionPropTypeId{148, NamespaceIndexes::SCADA};
const scada::NodeId kObjectStalePeriodTypeId{149, NamespaceIndexes::SCADA};
const scada::NodeId kObjectHistoricalDbPropTypeId{152, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId kTsInvertedPropTypeId{153, NamespaceIndexes::SCADA};
const scada::NodeId kTitConversionPropTypeId{156, NamespaceIndexes::SCADA};
const scada::NodeId kTitClampingPropTypeId{157, NamespaceIndexes::SCADA};
const scada::NodeId kTitEuLoPropTypeId{158, NamespaceIndexes::SCADA};
const scada::NodeId kTitEuHiPropTypeId{159, NamespaceIndexes::SCADA};
const scada::NodeId kTitIrLoPropTypeId{160, NamespaceIndexes::SCADA};
const scada::NodeId kTitIrHiPropTypeId{161, NamespaceIndexes::SCADA};
const scada::NodeId kTitLimitLoPropTypeId{162, NamespaceIndexes::SCADA};
const scada::NodeId kTitLimitHiPropTypeId{163, NamespaceIndexes::SCADA};
const scada::NodeId kTitLimitLoLoPropTypeId{164, NamespaceIndexes::SCADA};
const scada::NodeId kTitLimitHiHiPropTypeId{165, NamespaceIndexes::SCADA};
const scada::NodeId kUserAccessRightsPropTypeId{170, NamespaceIndexes::SCADA};
const scada::NodeId kHistoricalDbDepthPropTypeId{171, NamespaceIndexes::SCADA};
const scada::NodeId kSimulationSignalTypePropTypeId{172, NamespaceIndexes::SCADA};
const scada::NodeId kSimulationSignalPeriodPropTypeId{173, NamespaceIndexes::SCADA};
const scada::NodeId kSimulationSignalPhasePropTypeId{174, NamespaceIndexes::SCADA};
const scada::NodeId kSimulationSignalUpdateIntervalPropTypeId{175, NamespaceIndexes::SCADA};
const scada::NodeId kTsFormatOpenColorPropTypeId{218, NamespaceIndexes::SCADA};
const scada::NodeId kTsFormatCloseColorPropTypeId{219, NamespaceIndexes::SCADA};
const scada::NodeId kServerParamValuePropTypeId{220, NamespaceIndexes::SCADA};
const scada::NodeId kIecTransmitSourceRefTypeId{221, NamespaceIndexes::SCADA};
const scada::NodeId kIecTransmitSubsPropTypeId{223, NamespaceIndexes::SCADA};
const scada::NodeId kIecTransmitTargetDeviceRefTypeId{224, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId kIecTransmitTargetInfoAddressPropTypeId{225, NamespaceIndexes::SCADA};

const scada::NodeId kServerCpuConsumptionNodeId{226, NamespaceIndexes::SCADA};
const scada::NodeId kServerMemoryConsumptionNodeId{227, NamespaceIndexes::SCADA};
const scada::NodeId kComputerCpuConsumptionNodeId{228, NamespaceIndexes::SCADA};
const scada::NodeId kComputerMemoryConsumptionNodeId{229, NamespaceIndexes::SCADA};

const scada::NodeId kSystemHistoricalDbNodeId{233, NamespaceIndexes::SCADA};
const scada::NodeId kRootUserNodeId{234, NamespaceIndexes::SCADA};

const scada::NodeId kTypeMandatoryPropTypeId{236, NamespaceIndexes::SCADA};
const scada::NodeId kVariableTypeDefaultValuePropTypeId{237, NamespaceIndexes::SCADA};

const scada::NodeId kHistoricalDb_WriteValueDuration{240, NamespaceIndexes::SCADA};
const scada::NodeId kHistoricalDb_PendingTaskCount{241, NamespaceIndexes::SCADA};
const scada::NodeId kHistoricalDb_EventCleanupDuration{242, NamespaceIndexes::SCADA};
const scada::NodeId kHistoricalDb_ValueCleanupDuration{243, NamespaceIndexes::SCADA};

const scada::NodeId kServer{256, NamespaceIndexes::SCADA};
const scada::NodeId kServer_ProcessMemory{244, NamespaceIndexes::SCADA};
const scada::NodeId kServer_TotalMemory{245, NamespaceIndexes::SCADA};
const scada::NodeId kServer_ProcessCPU{246, NamespaceIndexes::SCADA};
const scada::NodeId kServer_TotalCPU{247, NamespaceIndexes::SCADA};

// Device
const scada::NodeId kDeviceOnlineVarTypeId{235, NamespaceIndexes::SCADA};
const scada::NodeId kDeviceStartCommandId{131, NamespaceIndexes::SCADA};
const scada::NodeId kDeviceStopCommandId{132, NamespaceIndexes::SCADA};
const scada::NodeId kDeviceInterrogateCommandId{133, NamespaceIndexes::SCADA};
const scada::NodeId kDeviceClockSyncCommandId{134, NamespaceIndexes::SCADA};
const scada::NodeId kChannelInterrogateCommandId{135, NamespaceIndexes::SCADA};

// Link
const scada::NodeId kLink_ConnectCount{250, NamespaceIndexes::SCADA};
const scada::NodeId kLink_ActiveConnections{251, NamespaceIndexes::SCADA};
const scada::NodeId kLink_MessagesIn{252, NamespaceIndexes::SCADA};
const scada::NodeId kLink_MessagesOut{253, NamespaceIndexes::SCADA};
const scada::NodeId kLink_BytesIn{254, NamespaceIndexes::SCADA};
const scada::NodeId kLink_BytesOut{255, NamespaceIndexes::SCADA};
const scada::NodeId kLinkTypeId{116, NamespaceIndexes::SCADA}; // :Device
const scada::NodeId kLinkTransportStringPropTypeId{142, NamespaceIndexes::SCADA};

// Iec60870Link
const scada::NodeId kIec60870LinkModePropTypeId{177, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkSendQueueSizePropTypeId{178, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkReceiveQueueSizePropTypeId{179, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkT0PropTypeId{180, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkConfirmationTimeoutPropTypeId{181, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkTerminationTimeoutProtocolPropTypeId{182, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkDeviceAddressSizePropTypeId{183, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkCotSizePropTypeId{184, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkInfoAddressSizePropTypeId{185, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkCollectDataPropTypeId{186, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkSendRetriesPropTypeId{187, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkCrcProtectionPropTypeId{188, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkSendTimeoutPropTypeId{189, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkReceiveTimeoutPropTypeId{190, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkIdleTimeoutPropTypeId{191, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870LinkAnonymousPropTypeId{192, NamespaceIndexes::SCADA};

// Iec60870Device
const scada::NodeId kIec60870DeviceAddressPropTypeId{193, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceLinkAddressPropTypeId{194, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceInterrogateOnStartPropTypeId{195, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceInterrogationPeriodPropTypeId{196, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceSynchronizeClockOnStartPropTypeId{197, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceClockSynchronizationPeriodPropTypeId{198, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup1PollPeriodPropTypeId{numeric_id::kIec60870DeviceGroup1PollPeriodPropTypeId, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup2PollPeriodPropTypeId{201, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup3PollPeriodPropTypeId{202, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup4PollPeriodPropTypeId{203, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup5PollPeriodPropTypeId{204, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup6PollPeriodPropTypeId{205, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup7PollPeriodPropTypeId{206, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup8PollPeriodPropTypeId{207, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup9PollPeriodPropTypeId{208, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup10PollPeriodPropTypeId{209, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup11PollPeriodPropTypeId{210, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup12PollPeriodPropTypeId{211, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup13PollPeriodPropTypeId{212, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup14PollPeriodPropTypeId{213, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup15PollPeriodPropTypeId{214, NamespaceIndexes::SCADA};
const scada::NodeId kIec60870DeviceGroup16PollPeriodPropTypeId{215, NamespaceIndexes::SCADA};

// ModbusPort
const scada::NodeId kModbusPortModeTypeId{130, NamespaceIndexes::SCADA};
const scada::NodeId kModbusPortModePropTypeId{167, NamespaceIndexes::SCADA};

// ModbusDevice
const scada::NodeId kModbusDeviceAddressPropTypeId{168, NamespaceIndexes::SCADA};
const scada::NodeId kModbusDeviceRepeatCountPropTypeId{169, NamespaceIndexes::SCADA};

namespace id {

const scada::NodeId Creates{297, NamespaceIndexes::SCADA}; // Ref

const scada::NodeId HasTsFormat{154, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId HasSimulationSignal{150, NamespaceIndexes::SCADA}; // Ref

const scada::NodeId TsFormats{numeric_id::TsFormats, NamespaceIndexes::SCADA};
const scada::NodeId DataItems{24, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabases{numeric_id::HistoricalDatabases, NamespaceIndexes::SCADA};
const scada::NodeId Users{numeric_id::Users, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignals{numeric_id::SimulationSignals, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItems{33, NamespaceIndexes::SCADA};

const scada::NodeId HistoricalDatabaseType{20, NamespaceIndexes::SCADA};
const scada::NodeId UserType{16, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType{67, NamespaceIndexes::SCADA};
const scada::NodeId DataGroupType{62, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType{80, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItemType{34, NamespaceIndexes::SCADA};
const scada::NodeId Devices{30, NamespaceIndexes::SCADA};

const scada::NodeId DataGroupType_Simulated{292, NamespaceIndexes::SCADA};

const scada::NodeId DataItemType{239, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Alias{140, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Locked{151, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Simulated{143, NamespaceIndexes::SCADA};

const scada::NodeId DiscreteItemType{72, NamespaceIndexes::SCADA}; // :DataItemType

const scada::NodeId AnalogItemType{76, NamespaceIndexes::SCADA}; // :DataItemType
const scada::NodeId AnalogItemType_DisplayFormat{155, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EngineeringUnits{166, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Aperture{295, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Deadband{296, NamespaceIndexes::SCADA};

const scada::NodeId TsFormatType_OpenLabel{216, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_CloseLabel{217, NamespaceIndexes::SCADA};

// DeviceType
const scada::NodeId DeviceType{115, NamespaceIndexes::SCADA}; // :BaseObjectType
const scada::NodeId DeviceType_Disabled{141, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Online{248, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Enabled{249, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Write{136, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Select{137, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_WriteParam{138, NamespaceIndexes::SCADA};

// Iec60870DeviceType
const scada::NodeId Iec60870DeviceType{331, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_UtcTime{294, NamespaceIndexes::SCADA};

// Iec60870LinkType
const scada::NodeId Iec60870LinkType{124, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Protocol{176, NamespaceIndexes::SCADA};

// Iec61850DeviceType
const scada::NodeId Iec61850DeviceType{277, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Host{278, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Port{279, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Model{288, NamespaceIndexes::SCADA};

// Iec61850ConfigurableObjectType : BaseObjectType
const scada::NodeId Iec61850ConfigurableObjectType{285, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850ConfigurableObjectType_Reference{283, NamespaceIndexes::SCADA};

// ModbusLinkType
const scada::NodeId ModbusLinkType{108, NamespaceIndexes::SCADA};

// ModbusDeviceType
const scada::NodeId ModbusDeviceType{118, NamespaceIndexes::SCADA};

// Iec61850LogicalNodeType : BaseObjectType
const scada::NodeId Iec61850LogicalNodeType{289, NamespaceIndexes::SCADA};

// Iec61850DataVariableType : BaseObjectType
const scada::NodeId Iec61850DataVariableType{291, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850ControlObjectType{293, NamespaceIndexes::SCADA};

// Iec61850RcbType : Iec61850ConfigurableObjectType
const scada::NodeId Iec61850RcbType{281, NamespaceIndexes::SCADA};

// Iec61850DataItemType
// Unused.
const scada::NodeId Iec61850DataItemType{286, NamespaceIndexes::SCADA};

const scada::NodeId NextId{298, NamespaceIndexes::SCADA};

} // namespace id