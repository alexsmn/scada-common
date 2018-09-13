#pragma once

#include "core/standard_node_ids.h"
#include "common/namespaces.h"

namespace numeric_id {

const scada::NumericId HistoricalDatabases = 12;
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup1 = 200;
const scada::NumericId SimulationSignals = 28;
const scada::NumericId TsFormats = 27;
const scada::NumericId Users = 29;

} // numeric_id

namespace id {

const scada::NodeId AnalogItemType_Aperture{295, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Clamping{157, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Conversion{156, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Deadband{296, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_DisplayFormat{155, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EngineeringUnits{166, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EuHi{159, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EuLo{158, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_IrHi{161, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_IrLo{160, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitHiHi{165, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitHi{163, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitLoLo{164, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitLo{162, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType{76, NamespaceIndexes::SCADA}; // :DataItemType
const scada::NodeId Creates{297, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId DataGroupType_Simulated{292, NamespaceIndexes::SCADA};
const scada::NodeId DataGroupType{62, NamespaceIndexes::SCADA};
const scada::NodeId DataItems{24, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Alias{140, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Input1{145, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Input2{146, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Locked{151, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_OutputCondition{148, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Output{147, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Severity{144, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Simulated{143, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_StalePeriod{149, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_WriteManual{230, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType{239, NamespaceIndexes::SCADA};
const scada::NodeId Devices{30, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Disabled{141, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Enabled{249, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_InterrogateChannel{135, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Interrogate{133, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Online{248, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Select{137, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_SyncClock{134, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_WriteParam{138, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Write{136, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType{115, NamespaceIndexes::SCADA}; // :BaseObjectType
const scada::NodeId DiscreteItemType_Inversion{153, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType{72, NamespaceIndexes::SCADA}; // :DataItemType
const scada::NodeId HasHistoricalDatabase{152, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId HasSimulationSignal{150, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId HasTransmissionSource{221, NamespaceIndexes::SCADA};
const scada::NodeId HasTransmissionTarget{224, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId HasTsFormat{154, NamespaceIndexes::SCADA}; // Ref
const scada::NodeId HistoricalDatabases{numeric_id::HistoricalDatabases, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_Depth{171, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_EventCleanupDuration{242, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_PendingTaskCount{241, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_ValueCleanupDuration{243, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_WriteValueDuration{240, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType{20, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Address{193, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_ClockSyncPeriod{198, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup10{209, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup11{210, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup12{211, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup13{212, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup14{213, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup15{214, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup16{215, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup1{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup1, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup2{201, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup3{202, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup4{203, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup5{204, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup6{205, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup7{206, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup8{207, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup9{208, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriod{196, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_LinkAddress{194, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_StartupClockSync{197, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_StartupInterrogation{195, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_UtcTime{294, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType{331, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_AnonymousMode{192, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ConfirmationTimeout{181, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ConnectTimeout{180, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_COTSize{184, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_CRCProtection{188, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_DataCollection{186, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_DeviceAddressSize{183, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_IdleTimeout{191, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_InfoAddressSize{185, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Mode{177, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Protocol{176, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ReceiveQueueSize{179, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ReceiveTimeout{190, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendQueueSize{178, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendRetryCount{187, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendTimeout{189, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_TerminationTimeout{182, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType{124, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850ConfigurableObjectType_Reference{283, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850ConfigurableObjectType{285, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850ControlObjectType{293, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DataVariableType{291, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Host{278, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Model{288, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType_Port{279, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850DeviceType{277, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850LogicalNodeType{289, NamespaceIndexes::SCADA};
const scada::NodeId Iec61850RcbType{281, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_ActiveConnections{251, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_BytesIn{254, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_BytesOut{255, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_ConnectCount{250, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_MessagesIn{252, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_MessagesOut{253, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Transport{142, NamespaceIndexes::SCADA};
const scada::NodeId LinkType{116, NamespaceIndexes::SCADA}; // :Device
const scada::NodeId ModbusDeviceType_Address{168, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_SendRetryCount{169, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_ResponseTimeout{298, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType{118, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Mode{130, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Protocol{167, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType{108, NamespaceIndexes::SCADA};
const scada::NodeId RootUser{234, NamespaceIndexes::SCADA};
const scada::NodeId Server_CPU{226, NamespaceIndexes::SCADA};
const scada::NodeId Server_Memory{227, NamespaceIndexes::SCADA};
const scada::NodeId Server_TotalCPU{228, NamespaceIndexes::SCADA};
const scada::NodeId Server_TotalMemory{229, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignals{numeric_id::SimulationSignals, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Period{173, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Phase{174, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Type{172, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_UpdateInterval{175, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType{67, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase{233, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItems{33, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItemType_SourceAddress{225, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItemType{34, NamespaceIndexes::SCADA};
const scada::NodeId TsFormats{numeric_id::TsFormats, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_CloseColor{219, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_CloseLabel{217, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_OpenColor{218, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_OpenLabel{216, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType{80, NamespaceIndexes::SCADA};
const scada::NodeId Users{numeric_id::Users, NamespaceIndexes::SCADA};
const scada::NodeId UserType_AccessRights{170, NamespaceIndexes::SCADA};
const scada::NodeId UserType{16, NamespaceIndexes::SCADA};

const scada::NodeId NextId{299, NamespaceIndexes::SCADA};

} // namespace id
