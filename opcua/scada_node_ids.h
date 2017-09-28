#pragma once

#include "core/namespaces.h"
#include "core/standard_node_ids.h"

namespace numeric_id {

const scada::NumericId HistoricalDatabases = 12; // Object
const scada::NumericId SystemDatabase = 13; // Object
const scada::NumericId UserType = 16; // ObjectType
const scada::NumericId UserType_AccessRights = 17; // Variable
const scada::NumericId DataItemType = 18; // VariableType
const scada::NumericId DataItemType_Alias = 19; // Variable
const scada::NumericId HistoricalDatabaseType = 20; // ObjectType
const scada::NumericId Creates = 23; // ReferenceType
const scada::NumericId DataItems = 24; // Object
const scada::NumericId TsFormats = 27; // Object
const scada::NumericId SimulationSignals = 28; // Object
const scada::NumericId Users = 29; // Object
const scada::NumericId Devices = 30; // Object
const scada::NumericId DeviceType = 31; // ObjectType
const scada::NumericId TransmissionItems = 33; // Object
const scada::NumericId TransmissionItemType = 34; // ObjectType
const scada::NumericId TransmissionItemType_SourceAddress = 35; // Variable
const scada::NumericId HistoricalDatabaseType_Depth = 36; // Variable
const scada::NumericId SystemDatabase_Depth = 37; // Variable
const scada::NumericId RootUser = 38; // Object
const scada::NumericId RootUser_AccessRights = 39; // Variable
const scada::NumericId LinkType = 40; // ObjectType
const scada::NumericId LinkType_Transport = 42; // Variable
const scada::NumericId LinkType_ConnectCount = 43; // Variable
const scada::NumericId LinkType_ActiveConnections = 44; // Variable
const scada::NumericId LinkType_MessagesOut = 45; // Variable
const scada::NumericId LinkType_MessagesIn = 46; // Variable
const scada::NumericId LinkType_BytesOut = 47; // Variable
const scada::NumericId LinkType_BytesIn = 48; // Variable
const scada::NumericId HistoricalDatabaseType_WriteValueDuration = 49; // Variable
const scada::NumericId HistoricalDatabaseType_PendingTaskCount = 50; // Variable
const scada::NumericId HistoricalDatabaseType_EventCleanupDuration = 51; // Variable
const scada::NumericId HistoricalDatabaseType_ValueCleanupDuration = 52; // Variable
const scada::NumericId SystemDatabase_WriteValueDuration = 53; // Variable
const scada::NumericId SystemDatabase_PendingTaskCount = 54; // Variable
const scada::NumericId SystemDatabase_EventCleanupDuration = 55; // Variable
const scada::NumericId SystemDatabase_ValueCleanupDuration = 56; // Variable
const scada::NumericId DataGroupType = 62; // ObjectType
const scada::NumericId DataGroupType_Simulated = 63; // Variable
const scada::NumericId DataItemType_Simulated = 66; // Variable
const scada::NumericId SimulationSignalType = 67; // VariableType
const scada::NumericId HasSimulationSignal = 69; // ReferenceType
const scada::NumericId HasHistoricalDatabase = 70; // ReferenceType
const scada::NumericId HasTsFormat = 71; // ReferenceType
const scada::NumericId DiscreteItemType = 72; // VariableType
const scada::NumericId DiscreteItemType_Alias = 73; // Variable
const scada::NumericId DiscreteItemType_Simulated = 74; // Variable
const scada::NumericId AnalogItemType = 76; // VariableType
const scada::NumericId TsFormatType = 80; // ObjectType
const scada::NumericId TsFormatType_OpenLabel = 81; // Variable
const scada::NumericId TsFormatType_CloseLabel = 82; // Variable
const scada::NumericId TsFormatType_OpenColor = 83; // Variable
const scada::NumericId TsFormatType_CloseColor = 84; // Variable
const scada::NumericId ModbusLinkModeType = 96; // DataType
const scada::NumericId ModbusLinkModeType_EnumValues = 97; // Variable
const scada::NumericId Scada_XmlSchema = 98; // Variable
const scada::NumericId Scada_XmlSchema_DataTypeVersion = 99; // Variable
const scada::NumericId Scada_XmlSchema_NamespaceUri = 100; // Variable
const scada::NumericId Scada_BinarySchema = 101; // Variable
const scada::NumericId Scada_BinarySchema_DataTypeVersion = 102; // Variable
const scada::NumericId Scada_BinarySchema_NamespaceUri = 103; // Variable
const scada::NumericId ModbusLinkType = 108; // ObjectType
const scada::NumericId ModbusLinkType_Transport = 110; // Variable
const scada::NumericId ModbusLinkType_ConnectCount = 111; // Variable
const scada::NumericId ModbusLinkType_ActiveConnections = 112; // Variable
const scada::NumericId ModbusLinkType_MessagesOut = 113; // Variable
const scada::NumericId ModbusLinkType_MessagesIn = 114; // Variable
const scada::NumericId ModbusLinkType_BytesOut = 115; // Variable
const scada::NumericId ModbusLinkType_BytesIn = 116; // Variable
const scada::NumericId ModbusLinkType_Mode = 117; // Variable
const scada::NumericId ModbusDeviceType = 118; // ObjectType
const scada::NumericId ModbusDeviceType_Address = 120; // Variable
const scada::NumericId ModbusDeviceType_SendRetryCount = 121; // Variable
const scada::NumericId HasTransmissionSource = 172; // ReferenceType
const scada::NumericId HasTransmissionTarget = 173; // ReferenceType
const scada::NumericId DataItemType_Severity = 174; // Variable
const scada::NumericId DataItemType_Input1 = 175; // Variable
const scada::NumericId DataItemType_Input2 = 176; // Variable
const scada::NumericId DataItemType_Output = 177; // Variable
const scada::NumericId DataItemType_OutputCondition = 178; // Variable
const scada::NumericId DataItemType_StalePeriod = 179; // Variable
const scada::NumericId DataItemType_Locked = 180; // Variable
const scada::NumericId DiscreteItemType_Severity = 181; // Variable
const scada::NumericId DiscreteItemType_Input1 = 182; // Variable
const scada::NumericId DiscreteItemType_Input2 = 183; // Variable
const scada::NumericId DiscreteItemType_Output = 184; // Variable
const scada::NumericId DiscreteItemType_OutputCondition = 185; // Variable
const scada::NumericId DiscreteItemType_StalePeriod = 186; // Variable
const scada::NumericId DiscreteItemType_Locked = 187; // Variable
const scada::NumericId DiscreteItemType_Inversion = 188; // Variable
const scada::NumericId AnalogItemType_DisplayFormat = 189; // Variable
const scada::NumericId AnalogItemType_Conversion = 190; // Variable
const scada::NumericId AnalogItemType_Clamping = 191; // Variable
const scada::NumericId AnalogItemType_EuLo = 192; // Variable
const scada::NumericId AnalogItemType_EuHi = 193; // Variable
const scada::NumericId AnalogItemType_IrLo = 194; // Variable
const scada::NumericId AnalogItemType_IrHi = 195; // Variable
const scada::NumericId AnalogItemType_LimitLo = 196; // Variable
const scada::NumericId AnalogItemType_LimitHi = 197; // Variable
const scada::NumericId AnalogItemType_LimitLoLo = 198; // Variable
const scada::NumericId AnalogItemType_LimitHiHi = 199; // Variable
const scada::NumericId AnalogItemType_EngineeringUnits = 200; // Variable
const scada::NumericId DeviceType_Disabled = 201; // Variable
const scada::NumericId DeviceType_Online = 202; // Variable
const scada::NumericId DeviceType_Enabled = 203; // Variable
const scada::NumericId LinkType_Disabled = 204; // Variable
const scada::NumericId LinkType_Online = 205; // Variable
const scada::NumericId LinkType_Enabled = 206; // Variable
const scada::NumericId ModbusLinkType_Disabled = 213; // Variable
const scada::NumericId ModbusLinkType_Online = 214; // Variable
const scada::NumericId ModbusLinkType_Enabled = 215; // Variable
const scada::NumericId ModbusDeviceType_Disabled = 216; // Variable
const scada::NumericId ModbusDeviceType_Online = 217; // Variable
const scada::NumericId ModbusDeviceType_Enabled = 218; // Variable
const scada::NumericId DeviceWatchEventType = 219; // ObjectType
const scada::NumericId DeviceWatchEventType_EventId = 220; // Variable
const scada::NumericId DeviceWatchEventType_EventType = 221; // Variable
const scada::NumericId DeviceWatchEventType_SourceNode = 222; // Variable
const scada::NumericId DeviceWatchEventType_SourceName = 223; // Variable
const scada::NumericId DeviceWatchEventType_Time = 224; // Variable
const scada::NumericId DeviceWatchEventType_ReceiveTime = 225; // Variable
const scada::NumericId DeviceWatchEventType_LocalTime = 226; // Variable
const scada::NumericId DeviceWatchEventType_Message = 227; // Variable
const scada::NumericId DeviceWatchEventType_Severity = 228; // Variable
const scada::NumericId SimulationSignalType_Type = 229; // Variable
const scada::NumericId SimulationSignalType_Period = 230; // Variable
const scada::NumericId SimulationSignalType_Phase = 231; // Variable
const scada::NumericId SimulationSignalType_UpdateInterval = 232; // Variable
const scada::NumericId DeviceInterrogateMethodType = 233; // Method
const scada::NumericId DeviceSyncClockMethodType = 234; // Method
const scada::NumericId DeviceType_Interrogate = 235; // Method
const scada::NumericId DeviceType_SyncClock = 236; // Method
const scada::NumericId LinkType_Interrogate = 237; // Method
const scada::NumericId LinkType_SyncClock = 238; // Method
const scada::NumericId ModbusLinkType_Interrogate = 243; // Method
const scada::NumericId ModbusLinkType_SyncClock = 244; // Method
const scada::NumericId ModbusDeviceType_Interrogate = 245; // Method
const scada::NumericId ModbusDeviceType_SyncClock = 246; // Method
const scada::NumericId DeviceType_Select = 256; // Method
const scada::NumericId DeviceType_Write = 257; // Method
const scada::NumericId DeviceType_WriteParam = 258; // Method
const scada::NumericId LinkType_Select = 259; // Method
const scada::NumericId LinkType_Write = 260; // Method
const scada::NumericId LinkType_WriteParam = 261; // Method
const scada::NumericId ModbusLinkType_Select = 268; // Method
const scada::NumericId ModbusLinkType_Write = 269; // Method
const scada::NumericId ModbusLinkType_WriteParam = 270; // Method
const scada::NumericId ModbusDeviceType_Select = 271; // Method
const scada::NumericId ModbusDeviceType_Write = 272; // Method
const scada::NumericId ModbusDeviceType_WriteParam = 273; // Method
const scada::NumericId DeviceSelectMethodType = 274; // Method
const scada::NumericId DeviceWriteMethodType = 275; // Method
const scada::NumericId DeviceWriteParamMethodType = 276; // Method
const scada::NumericId DeviceInterrogateChannelMethodType = 277; // Method
const scada::NumericId DeviceType_InterrogateChannel = 278; // Method
const scada::NumericId LinkType_InterrogateChannel = 279; // Method
const scada::NumericId ModbusLinkType_InterrogateChannel = 282; // Method
const scada::NumericId ModbusDeviceType_InterrogateChannel = 283; // Method
const scada::NumericId DataItemWriteManualMethodType = 284; // Method
const scada::NumericId DataItemType_WriteManual = 285; // Method
const scada::NumericId DiscreteItemType_WriteManual = 286; // Method
const scada::NumericId AnalogItemType_Alias = 287; // Variable
const scada::NumericId AnalogItemType_Severity = 288; // Variable
const scada::NumericId AnalogItemType_Input1 = 289; // Variable
const scada::NumericId AnalogItemType_Input2 = 290; // Variable
const scada::NumericId AnalogItemType_Output = 291; // Variable
const scada::NumericId AnalogItemType_OutputCondition = 292; // Variable
const scada::NumericId AnalogItemType_StalePeriod = 293; // Variable
const scada::NumericId AnalogItemType_Simulated = 294; // Variable
const scada::NumericId AnalogItemType_Locked = 295; // Variable
const scada::NumericId AnalogItemType_WriteManual = 296; // Method
const scada::NumericId Iec60870LinkType = 297; // ObjectType
const scada::NumericId Iec60870LinkType_Disabled = 298; // Variable
const scada::NumericId Iec60870LinkType_Online = 299; // Variable
const scada::NumericId Iec60870LinkType_Enabled = 300; // Variable
const scada::NumericId Iec60870LinkType_Interrogate = 301; // Method
const scada::NumericId Iec60870LinkType_InterrogateChannel = 302; // Method
const scada::NumericId Iec60870LinkType_SyncClock = 303; // Method
const scada::NumericId Iec60870LinkType_Select = 304; // Method
const scada::NumericId Iec60870LinkType_Write = 305; // Method
const scada::NumericId Iec60870LinkType_WriteParam = 306; // Method
const scada::NumericId Iec60870LinkType_Transport = 307; // Variable
const scada::NumericId Iec60870LinkType_ConnectCount = 308; // Variable
const scada::NumericId Iec60870LinkType_ActiveConnections = 309; // Variable
const scada::NumericId Iec60870LinkType_MessagesOut = 310; // Variable
const scada::NumericId Iec60870LinkType_MessagesIn = 311; // Variable
const scada::NumericId Iec60870LinkType_BytesOut = 312; // Variable
const scada::NumericId Iec60870LinkType_BytesIn = 313; // Variable
const scada::NumericId Iec60870LinkType_Protocol = 314; // Variable
const scada::NumericId Iec60870LinkType_Mode = 315; // Variable
const scada::NumericId Iec60870LinkType_SendQueueSize = 316; // Variable
const scada::NumericId Iec60870LinkType_ReceiveQueueSize = 317; // Variable
const scada::NumericId Iec60870LinkType_ConnectTimeout = 318; // Variable
const scada::NumericId Iec60870LinkType_ConfirmationTimeout = 319; // Variable
const scada::NumericId Iec60870LinkType_TerminationTimeout = 320; // Variable
const scada::NumericId Iec60870LinkType_DeviceAddressSize = 321; // Variable
const scada::NumericId Iec60870LinkType_COTSize = 322; // Variable
const scada::NumericId Iec60870LinkType_InfoAddressSize = 323; // Variable
const scada::NumericId Iec60870LinkType_DataCollection = 324; // Variable
const scada::NumericId Iec60870LinkType_SendRetryCount = 325; // Variable
const scada::NumericId Iec60870LinkType_CRCProtection = 326; // Variable
const scada::NumericId Iec60870LinkType_SendTimeout = 327; // Variable
const scada::NumericId Iec60870LinkType_ReceiveTimeout = 328; // Variable
const scada::NumericId Iec60870LinkType_IdleTimeout = 329; // Variable
const scada::NumericId Iec60870LinkType_AnonymousMode = 330; // Variable
const scada::NumericId Iec60870DeviceType = 331; // ObjectType
const scada::NumericId Iec60870DeviceType_Disabled = 332; // Variable
const scada::NumericId Iec60870DeviceType_Online = 333; // Variable
const scada::NumericId Iec60870DeviceType_Enabled = 334; // Variable
const scada::NumericId Iec60870DeviceType_Interrogate = 335; // Method
const scada::NumericId Iec60870DeviceType_InterrogateChannel = 336; // Method
const scada::NumericId Iec60870DeviceType_SyncClock = 337; // Method
const scada::NumericId Iec60870DeviceType_Select = 338; // Method
const scada::NumericId Iec60870DeviceType_Write = 339; // Method
const scada::NumericId Iec60870DeviceType_WriteParam = 340; // Method
const scada::NumericId Iec60870DeviceType_Address = 341; // Variable
const scada::NumericId Iec60870DeviceType_LinkAddress = 342; // Variable
const scada::NumericId Iec60870DeviceType_StartupInterrogation = 343; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriod = 344; // Variable
const scada::NumericId Iec60870DeviceType_StartupClockSync = 345; // Variable
const scada::NumericId Iec60870DeviceType_ClockSyncPeriod = 346; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup1 = 347; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup2 = 348; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup3 = 349; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup4 = 350; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup5 = 351; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup6 = 352; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup7 = 353; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup8 = 354; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup9 = 355; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup10 = 356; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup11 = 357; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup12 = 358; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup13 = 359; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup14 = 360; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup15 = 361; // Variable
const scada::NumericId Iec60870DeviceType_InterrogationPeriodGroup16 = 362; // Variable
const scada::NumericId Statistics = 363; // Object
const scada::NumericId Statistics_ServerCPUUsage = 366; // Variable
const scada::NumericId Statistics_ServerMemoryUsage = 367; // Variable
const scada::NumericId Statistics_TotalCPUUsage = 368; // Variable
const scada::NumericId Statistics_TotalMemoryUsage = 369; // Variable

} // numeric_id

namespace id {

const scada::NodeId HistoricalDatabases{numeric_id::HistoricalDatabases, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase{numeric_id::SystemDatabase, NamespaceIndexes::SCADA};
const scada::NodeId UserType{numeric_id::UserType, NamespaceIndexes::SCADA};
const scada::NodeId UserType_AccessRights{numeric_id::UserType_AccessRights, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType{numeric_id::DataItemType, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Alias{numeric_id::DataItemType_Alias, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType{numeric_id::HistoricalDatabaseType, NamespaceIndexes::SCADA};
const scada::NodeId Creates{numeric_id::Creates, NamespaceIndexes::SCADA};
const scada::NodeId DataItems{numeric_id::DataItems, NamespaceIndexes::SCADA};
const scada::NodeId TsFormats{numeric_id::TsFormats, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignals{numeric_id::SimulationSignals, NamespaceIndexes::SCADA};
const scada::NodeId Users{numeric_id::Users, NamespaceIndexes::SCADA};
const scada::NodeId Devices{numeric_id::Devices, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType{numeric_id::DeviceType, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItems{numeric_id::TransmissionItems, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItemType{numeric_id::TransmissionItemType, NamespaceIndexes::SCADA};
const scada::NodeId TransmissionItemType_SourceAddress{numeric_id::TransmissionItemType_SourceAddress, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_Depth{numeric_id::HistoricalDatabaseType_Depth, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase_Depth{numeric_id::SystemDatabase_Depth, NamespaceIndexes::SCADA};
const scada::NodeId RootUser{numeric_id::RootUser, NamespaceIndexes::SCADA};
const scada::NodeId RootUser_AccessRights{numeric_id::RootUser_AccessRights, NamespaceIndexes::SCADA};
const scada::NodeId LinkType{numeric_id::LinkType, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Transport{numeric_id::LinkType_Transport, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_ConnectCount{numeric_id::LinkType_ConnectCount, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_ActiveConnections{numeric_id::LinkType_ActiveConnections, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_MessagesOut{numeric_id::LinkType_MessagesOut, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_MessagesIn{numeric_id::LinkType_MessagesIn, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_BytesOut{numeric_id::LinkType_BytesOut, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_BytesIn{numeric_id::LinkType_BytesIn, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_WriteValueDuration{numeric_id::HistoricalDatabaseType_WriteValueDuration, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_PendingTaskCount{numeric_id::HistoricalDatabaseType_PendingTaskCount, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_EventCleanupDuration{numeric_id::HistoricalDatabaseType_EventCleanupDuration, NamespaceIndexes::SCADA};
const scada::NodeId HistoricalDatabaseType_ValueCleanupDuration{numeric_id::HistoricalDatabaseType_ValueCleanupDuration, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase_WriteValueDuration{numeric_id::SystemDatabase_WriteValueDuration, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase_PendingTaskCount{numeric_id::SystemDatabase_PendingTaskCount, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase_EventCleanupDuration{numeric_id::SystemDatabase_EventCleanupDuration, NamespaceIndexes::SCADA};
const scada::NodeId SystemDatabase_ValueCleanupDuration{numeric_id::SystemDatabase_ValueCleanupDuration, NamespaceIndexes::SCADA};
const scada::NodeId DataGroupType{numeric_id::DataGroupType, NamespaceIndexes::SCADA};
const scada::NodeId DataGroupType_Simulated{numeric_id::DataGroupType_Simulated, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Simulated{numeric_id::DataItemType_Simulated, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType{numeric_id::SimulationSignalType, NamespaceIndexes::SCADA};
const scada::NodeId HasSimulationSignal{numeric_id::HasSimulationSignal, NamespaceIndexes::SCADA};
const scada::NodeId HasHistoricalDatabase{numeric_id::HasHistoricalDatabase, NamespaceIndexes::SCADA};
const scada::NodeId HasTsFormat{numeric_id::HasTsFormat, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType{numeric_id::DiscreteItemType, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Alias{numeric_id::DiscreteItemType_Alias, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Simulated{numeric_id::DiscreteItemType_Simulated, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType{numeric_id::AnalogItemType, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType{numeric_id::TsFormatType, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_OpenLabel{numeric_id::TsFormatType_OpenLabel, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_CloseLabel{numeric_id::TsFormatType_CloseLabel, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_OpenColor{numeric_id::TsFormatType_OpenColor, NamespaceIndexes::SCADA};
const scada::NodeId TsFormatType_CloseColor{numeric_id::TsFormatType_CloseColor, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkModeType{numeric_id::ModbusLinkModeType, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkModeType_EnumValues{numeric_id::ModbusLinkModeType_EnumValues, NamespaceIndexes::SCADA};
const scada::NodeId Scada_XmlSchema{numeric_id::Scada_XmlSchema, NamespaceIndexes::SCADA};
const scada::NodeId Scada_XmlSchema_DataTypeVersion{numeric_id::Scada_XmlSchema_DataTypeVersion, NamespaceIndexes::SCADA};
const scada::NodeId Scada_XmlSchema_NamespaceUri{numeric_id::Scada_XmlSchema_NamespaceUri, NamespaceIndexes::SCADA};
const scada::NodeId Scada_BinarySchema{numeric_id::Scada_BinarySchema, NamespaceIndexes::SCADA};
const scada::NodeId Scada_BinarySchema_DataTypeVersion{numeric_id::Scada_BinarySchema_DataTypeVersion, NamespaceIndexes::SCADA};
const scada::NodeId Scada_BinarySchema_NamespaceUri{numeric_id::Scada_BinarySchema_NamespaceUri, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType{numeric_id::ModbusLinkType, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Transport{numeric_id::ModbusLinkType_Transport, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_ConnectCount{numeric_id::ModbusLinkType_ConnectCount, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_ActiveConnections{numeric_id::ModbusLinkType_ActiveConnections, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_MessagesOut{numeric_id::ModbusLinkType_MessagesOut, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_MessagesIn{numeric_id::ModbusLinkType_MessagesIn, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_BytesOut{numeric_id::ModbusLinkType_BytesOut, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_BytesIn{numeric_id::ModbusLinkType_BytesIn, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Mode{numeric_id::ModbusLinkType_Mode, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType{numeric_id::ModbusDeviceType, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Address{numeric_id::ModbusDeviceType_Address, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_SendRetryCount{numeric_id::ModbusDeviceType_SendRetryCount, NamespaceIndexes::SCADA};
const scada::NodeId HasTransmissionSource{numeric_id::HasTransmissionSource, NamespaceIndexes::SCADA};
const scada::NodeId HasTransmissionTarget{numeric_id::HasTransmissionTarget, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Severity{numeric_id::DataItemType_Severity, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Input1{numeric_id::DataItemType_Input1, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Input2{numeric_id::DataItemType_Input2, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Output{numeric_id::DataItemType_Output, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_OutputCondition{numeric_id::DataItemType_OutputCondition, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_StalePeriod{numeric_id::DataItemType_StalePeriod, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_Locked{numeric_id::DataItemType_Locked, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Severity{numeric_id::DiscreteItemType_Severity, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Input1{numeric_id::DiscreteItemType_Input1, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Input2{numeric_id::DiscreteItemType_Input2, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Output{numeric_id::DiscreteItemType_Output, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_OutputCondition{numeric_id::DiscreteItemType_OutputCondition, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_StalePeriod{numeric_id::DiscreteItemType_StalePeriod, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Locked{numeric_id::DiscreteItemType_Locked, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_Inversion{numeric_id::DiscreteItemType_Inversion, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_DisplayFormat{numeric_id::AnalogItemType_DisplayFormat, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Conversion{numeric_id::AnalogItemType_Conversion, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Clamping{numeric_id::AnalogItemType_Clamping, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EuLo{numeric_id::AnalogItemType_EuLo, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EuHi{numeric_id::AnalogItemType_EuHi, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_IrLo{numeric_id::AnalogItemType_IrLo, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_IrHi{numeric_id::AnalogItemType_IrHi, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitLo{numeric_id::AnalogItemType_LimitLo, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitHi{numeric_id::AnalogItemType_LimitHi, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitLoLo{numeric_id::AnalogItemType_LimitLoLo, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_LimitHiHi{numeric_id::AnalogItemType_LimitHiHi, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_EngineeringUnits{numeric_id::AnalogItemType_EngineeringUnits, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Disabled{numeric_id::DeviceType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Online{numeric_id::DeviceType_Online, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Enabled{numeric_id::DeviceType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Disabled{numeric_id::LinkType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Online{numeric_id::LinkType_Online, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Enabled{numeric_id::LinkType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Disabled{numeric_id::ModbusLinkType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Online{numeric_id::ModbusLinkType_Online, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Enabled{numeric_id::ModbusLinkType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Disabled{numeric_id::ModbusDeviceType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Online{numeric_id::ModbusDeviceType_Online, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Enabled{numeric_id::ModbusDeviceType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType{numeric_id::DeviceWatchEventType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_EventId{numeric_id::DeviceWatchEventType_EventId, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_EventType{numeric_id::DeviceWatchEventType_EventType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_SourceNode{numeric_id::DeviceWatchEventType_SourceNode, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_SourceName{numeric_id::DeviceWatchEventType_SourceName, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_Time{numeric_id::DeviceWatchEventType_Time, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_ReceiveTime{numeric_id::DeviceWatchEventType_ReceiveTime, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_LocalTime{numeric_id::DeviceWatchEventType_LocalTime, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_Message{numeric_id::DeviceWatchEventType_Message, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWatchEventType_Severity{numeric_id::DeviceWatchEventType_Severity, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Type{numeric_id::SimulationSignalType_Type, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Period{numeric_id::SimulationSignalType_Period, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_Phase{numeric_id::SimulationSignalType_Phase, NamespaceIndexes::SCADA};
const scada::NodeId SimulationSignalType_UpdateInterval{numeric_id::SimulationSignalType_UpdateInterval, NamespaceIndexes::SCADA};
const scada::NodeId DeviceInterrogateMethodType{numeric_id::DeviceInterrogateMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceSyncClockMethodType{numeric_id::DeviceSyncClockMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Interrogate{numeric_id::DeviceType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_SyncClock{numeric_id::DeviceType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Interrogate{numeric_id::LinkType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_SyncClock{numeric_id::LinkType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Interrogate{numeric_id::ModbusLinkType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_SyncClock{numeric_id::ModbusLinkType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Interrogate{numeric_id::ModbusDeviceType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_SyncClock{numeric_id::ModbusDeviceType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Select{numeric_id::DeviceType_Select, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_Write{numeric_id::DeviceType_Write, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_WriteParam{numeric_id::DeviceType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Select{numeric_id::LinkType_Select, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_Write{numeric_id::LinkType_Write, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_WriteParam{numeric_id::LinkType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Select{numeric_id::ModbusLinkType_Select, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_Write{numeric_id::ModbusLinkType_Write, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_WriteParam{numeric_id::ModbusLinkType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Select{numeric_id::ModbusDeviceType_Select, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_Write{numeric_id::ModbusDeviceType_Write, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_WriteParam{numeric_id::ModbusDeviceType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId DeviceSelectMethodType{numeric_id::DeviceSelectMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWriteMethodType{numeric_id::DeviceWriteMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceWriteParamMethodType{numeric_id::DeviceWriteParamMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceInterrogateChannelMethodType{numeric_id::DeviceInterrogateChannelMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DeviceType_InterrogateChannel{numeric_id::DeviceType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId LinkType_InterrogateChannel{numeric_id::LinkType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId ModbusLinkType_InterrogateChannel{numeric_id::ModbusLinkType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId ModbusDeviceType_InterrogateChannel{numeric_id::ModbusDeviceType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId DataItemWriteManualMethodType{numeric_id::DataItemWriteManualMethodType, NamespaceIndexes::SCADA};
const scada::NodeId DataItemType_WriteManual{numeric_id::DataItemType_WriteManual, NamespaceIndexes::SCADA};
const scada::NodeId DiscreteItemType_WriteManual{numeric_id::DiscreteItemType_WriteManual, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Alias{numeric_id::AnalogItemType_Alias, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Severity{numeric_id::AnalogItemType_Severity, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Input1{numeric_id::AnalogItemType_Input1, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Input2{numeric_id::AnalogItemType_Input2, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Output{numeric_id::AnalogItemType_Output, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_OutputCondition{numeric_id::AnalogItemType_OutputCondition, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_StalePeriod{numeric_id::AnalogItemType_StalePeriod, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Simulated{numeric_id::AnalogItemType_Simulated, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_Locked{numeric_id::AnalogItemType_Locked, NamespaceIndexes::SCADA};
const scada::NodeId AnalogItemType_WriteManual{numeric_id::AnalogItemType_WriteManual, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType{numeric_id::Iec60870LinkType, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Disabled{numeric_id::Iec60870LinkType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Online{numeric_id::Iec60870LinkType_Online, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Enabled{numeric_id::Iec60870LinkType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Interrogate{numeric_id::Iec60870LinkType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_InterrogateChannel{numeric_id::Iec60870LinkType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SyncClock{numeric_id::Iec60870LinkType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Select{numeric_id::Iec60870LinkType_Select, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Write{numeric_id::Iec60870LinkType_Write, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_WriteParam{numeric_id::Iec60870LinkType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Transport{numeric_id::Iec60870LinkType_Transport, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ConnectCount{numeric_id::Iec60870LinkType_ConnectCount, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ActiveConnections{numeric_id::Iec60870LinkType_ActiveConnections, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_MessagesOut{numeric_id::Iec60870LinkType_MessagesOut, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_MessagesIn{numeric_id::Iec60870LinkType_MessagesIn, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_BytesOut{numeric_id::Iec60870LinkType_BytesOut, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_BytesIn{numeric_id::Iec60870LinkType_BytesIn, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Protocol{numeric_id::Iec60870LinkType_Protocol, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_Mode{numeric_id::Iec60870LinkType_Mode, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendQueueSize{numeric_id::Iec60870LinkType_SendQueueSize, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ReceiveQueueSize{numeric_id::Iec60870LinkType_ReceiveQueueSize, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ConnectTimeout{numeric_id::Iec60870LinkType_ConnectTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ConfirmationTimeout{numeric_id::Iec60870LinkType_ConfirmationTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_TerminationTimeout{numeric_id::Iec60870LinkType_TerminationTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_DeviceAddressSize{numeric_id::Iec60870LinkType_DeviceAddressSize, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_COTSize{numeric_id::Iec60870LinkType_COTSize, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_InfoAddressSize{numeric_id::Iec60870LinkType_InfoAddressSize, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_DataCollection{numeric_id::Iec60870LinkType_DataCollection, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendRetryCount{numeric_id::Iec60870LinkType_SendRetryCount, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_CRCProtection{numeric_id::Iec60870LinkType_CRCProtection, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_SendTimeout{numeric_id::Iec60870LinkType_SendTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_ReceiveTimeout{numeric_id::Iec60870LinkType_ReceiveTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_IdleTimeout{numeric_id::Iec60870LinkType_IdleTimeout, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870LinkType_AnonymousMode{numeric_id::Iec60870LinkType_AnonymousMode, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType{numeric_id::Iec60870DeviceType, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Disabled{numeric_id::Iec60870DeviceType_Disabled, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Online{numeric_id::Iec60870DeviceType_Online, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Enabled{numeric_id::Iec60870DeviceType_Enabled, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Interrogate{numeric_id::Iec60870DeviceType_Interrogate, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogateChannel{numeric_id::Iec60870DeviceType_InterrogateChannel, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_SyncClock{numeric_id::Iec60870DeviceType_SyncClock, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Select{numeric_id::Iec60870DeviceType_Select, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Write{numeric_id::Iec60870DeviceType_Write, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_WriteParam{numeric_id::Iec60870DeviceType_WriteParam, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_Address{numeric_id::Iec60870DeviceType_Address, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_LinkAddress{numeric_id::Iec60870DeviceType_LinkAddress, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_StartupInterrogation{numeric_id::Iec60870DeviceType_StartupInterrogation, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriod{numeric_id::Iec60870DeviceType_InterrogationPeriod, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_StartupClockSync{numeric_id::Iec60870DeviceType_StartupClockSync, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_ClockSyncPeriod{numeric_id::Iec60870DeviceType_ClockSyncPeriod, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup1{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup1, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup2{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup2, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup3{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup3, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup4{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup4, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup5{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup5, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup6{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup6, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup7{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup7, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup8{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup8, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup9{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup9, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup10{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup10, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup11{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup11, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup12{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup12, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup13{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup13, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup14{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup14, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup15{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup15, NamespaceIndexes::SCADA};
const scada::NodeId Iec60870DeviceType_InterrogationPeriodGroup16{numeric_id::Iec60870DeviceType_InterrogationPeriodGroup16, NamespaceIndexes::SCADA};
const scada::NodeId Statistics{numeric_id::Statistics, NamespaceIndexes::SCADA};
const scada::NodeId Statistics_ServerCPUUsage{numeric_id::Statistics_ServerCPUUsage, NamespaceIndexes::SCADA};
const scada::NodeId Statistics_ServerMemoryUsage{numeric_id::Statistics_ServerMemoryUsage, NamespaceIndexes::SCADA};
const scada::NodeId Statistics_TotalCPUUsage{numeric_id::Statistics_TotalCPUUsage, NamespaceIndexes::SCADA};
const scada::NodeId Statistics_TotalMemoryUsage{numeric_id::Statistics_TotalMemoryUsage, NamespaceIndexes::SCADA};

} // namespace id
