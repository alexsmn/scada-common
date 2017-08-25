/* ========================================================================
 * Copyright (c) 2005-2016 The OPC Foundation, Inc. All rights reserved.
 *
 * OPC Foundation MIT License 1.00
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The complete license agreement can be found here:
 * http://opcfoundation.org/License/MIT/1.00/
 * ======================================================================*/

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua;

namespace Telecontrol.Scada
{
    #region DataType Identifiers
    /// <summary>
    /// A class that declares constants for all DataTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class DataTypes
    {
        /// <summary>
        /// The identifier for the ModbusLinkModeType DataType.
        /// </summary>
        public const uint ModbusLinkModeType = 96;
    }
    #endregion

    #region Method Identifiers
    /// <summary>
    /// A class that declares constants for all Methods in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Methods
    {
        /// <summary>
        /// The identifier for the DataItemType_WriteManual Method.
        /// </summary>
        public const uint DataItemType_WriteManual = 285;

        /// <summary>
        /// The identifier for the DeviceType_Interrogate Method.
        /// </summary>
        public const uint DeviceType_Interrogate = 235;

        /// <summary>
        /// The identifier for the DeviceType_InterrogateChannel Method.
        /// </summary>
        public const uint DeviceType_InterrogateChannel = 278;

        /// <summary>
        /// The identifier for the DeviceType_SyncClock Method.
        /// </summary>
        public const uint DeviceType_SyncClock = 236;

        /// <summary>
        /// The identifier for the DeviceType_Select Method.
        /// </summary>
        public const uint DeviceType_Select = 256;

        /// <summary>
        /// The identifier for the DeviceType_Write Method.
        /// </summary>
        public const uint DeviceType_Write = 257;

        /// <summary>
        /// The identifier for the DeviceType_WriteParam Method.
        /// </summary>
        public const uint DeviceType_WriteParam = 258;
    }
    #endregion

    #region Object Identifiers
    /// <summary>
    /// A class that declares constants for all Objects in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Objects
    {
        /// <summary>
        /// The identifier for the DataItems Object.
        /// </summary>
        public const uint DataItems = 24;

        /// <summary>
        /// The identifier for the TsFormats Object.
        /// </summary>
        public const uint TsFormats = 27;

        /// <summary>
        /// The identifier for the SimulationSignals Object.
        /// </summary>
        public const uint SimulationSignals = 28;

        /// <summary>
        /// The identifier for the Users Object.
        /// </summary>
        public const uint Users = 29;

        /// <summary>
        /// The identifier for the RootUser Object.
        /// </summary>
        public const uint RootUser = 38;

        /// <summary>
        /// The identifier for the HistoricalDatabases Object.
        /// </summary>
        public const uint HistoricalDatabases = 12;

        /// <summary>
        /// The identifier for the SystemDatabase Object.
        /// </summary>
        public const uint SystemDatabase = 13;

        /// <summary>
        /// The identifier for the Devices Object.
        /// </summary>
        public const uint Devices = 30;

        /// <summary>
        /// The identifier for the TransmissionItems Object.
        /// </summary>
        public const uint TransmissionItems = 33;
    }
    #endregion

    #region ObjectType Identifiers
    /// <summary>
    /// A class that declares constants for all ObjectTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectTypes
    {
        /// <summary>
        /// The identifier for the DataGroupType ObjectType.
        /// </summary>
        public const uint DataGroupType = 62;

        /// <summary>
        /// The identifier for the TsFormatType ObjectType.
        /// </summary>
        public const uint TsFormatType = 80;

        /// <summary>
        /// The identifier for the UserType ObjectType.
        /// </summary>
        public const uint UserType = 16;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType ObjectType.
        /// </summary>
        public const uint HistoricalDatabaseType = 20;

        /// <summary>
        /// The identifier for the DeviceType ObjectType.
        /// </summary>
        public const uint DeviceType = 31;

        /// <summary>
        /// The identifier for the LinkType ObjectType.
        /// </summary>
        public const uint LinkType = 40;

        /// <summary>
        /// The identifier for the DeviceWatchEventType ObjectType.
        /// </summary>
        public const uint DeviceWatchEventType = 219;

        /// <summary>
        /// The identifier for the Iec60870LinkType ObjectType.
        /// </summary>
        public const uint Iec60870LinkType = 297;

        /// <summary>
        /// The identifier for the Iec60870DeviceType ObjectType.
        /// </summary>
        public const uint Iec60870DeviceType = 331;

        /// <summary>
        /// The identifier for the ModbusLinkType ObjectType.
        /// </summary>
        public const uint ModbusLinkType = 108;

        /// <summary>
        /// The identifier for the ModbusDeviceType ObjectType.
        /// </summary>
        public const uint ModbusDeviceType = 118;

        /// <summary>
        /// The identifier for the TransmissionItemType ObjectType.
        /// </summary>
        public const uint TransmissionItemType = 34;
    }
    #endregion

    #region ReferenceType Identifiers
    /// <summary>
    /// A class that declares constants for all ReferenceTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ReferenceTypes
    {
        /// <summary>
        /// The identifier for the Creates ReferenceType.
        /// </summary>
        public const uint Creates = 23;

        /// <summary>
        /// The identifier for the HasSimulationSignal ReferenceType.
        /// </summary>
        public const uint HasSimulationSignal = 69;

        /// <summary>
        /// The identifier for the HasHistoricalDatabase ReferenceType.
        /// </summary>
        public const uint HasHistoricalDatabase = 70;

        /// <summary>
        /// The identifier for the HasTsFormat ReferenceType.
        /// </summary>
        public const uint HasTsFormat = 71;

        /// <summary>
        /// The identifier for the HasTransmissionSource ReferenceType.
        /// </summary>
        public const uint HasTransmissionSource = 172;

        /// <summary>
        /// The identifier for the HasTransmissionTarget ReferenceType.
        /// </summary>
        public const uint HasTransmissionTarget = 173;
    }
    #endregion

    #region Variable Identifiers
    /// <summary>
    /// A class that declares constants for all Variables in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Variables
    {
        /// <summary>
        /// The identifier for the DataGroupType_Simulated Variable.
        /// </summary>
        public const uint DataGroupType_Simulated = 63;

        /// <summary>
        /// The identifier for the DataItemType_Alias Variable.
        /// </summary>
        public const uint DataItemType_Alias = 19;

        /// <summary>
        /// The identifier for the DataItemType_Severity Variable.
        /// </summary>
        public const uint DataItemType_Severity = 174;

        /// <summary>
        /// The identifier for the DataItemType_Input1 Variable.
        /// </summary>
        public const uint DataItemType_Input1 = 175;

        /// <summary>
        /// The identifier for the DataItemType_Input2 Variable.
        /// </summary>
        public const uint DataItemType_Input2 = 176;

        /// <summary>
        /// The identifier for the DataItemType_Output Variable.
        /// </summary>
        public const uint DataItemType_Output = 177;

        /// <summary>
        /// The identifier for the DataItemType_OutputCondition Variable.
        /// </summary>
        public const uint DataItemType_OutputCondition = 178;

        /// <summary>
        /// The identifier for the DataItemType_StalePeriod Variable.
        /// </summary>
        public const uint DataItemType_StalePeriod = 179;

        /// <summary>
        /// The identifier for the DataItemType_Simulated Variable.
        /// </summary>
        public const uint DataItemType_Simulated = 66;

        /// <summary>
        /// The identifier for the DataItemType_Locked Variable.
        /// </summary>
        public const uint DataItemType_Locked = 180;

        /// <summary>
        /// The identifier for the DiscreteItemType_Inversion Variable.
        /// </summary>
        public const uint DiscreteItemType_Inversion = 188;

        /// <summary>
        /// The identifier for the AnalogItemType_DisplayFormat Variable.
        /// </summary>
        public const uint AnalogItemType_DisplayFormat = 189;

        /// <summary>
        /// The identifier for the AnalogItemType_Conversion Variable.
        /// </summary>
        public const uint AnalogItemType_Conversion = 190;

        /// <summary>
        /// The identifier for the AnalogItemType_Clamping Variable.
        /// </summary>
        public const uint AnalogItemType_Clamping = 191;

        /// <summary>
        /// The identifier for the AnalogItemType_EuLo Variable.
        /// </summary>
        public const uint AnalogItemType_EuLo = 192;

        /// <summary>
        /// The identifier for the AnalogItemType_EuHi Variable.
        /// </summary>
        public const uint AnalogItemType_EuHi = 193;

        /// <summary>
        /// The identifier for the AnalogItemType_IrLo Variable.
        /// </summary>
        public const uint AnalogItemType_IrLo = 194;

        /// <summary>
        /// The identifier for the AnalogItemType_IrHi Variable.
        /// </summary>
        public const uint AnalogItemType_IrHi = 195;

        /// <summary>
        /// The identifier for the AnalogItemType_LimitLo Variable.
        /// </summary>
        public const uint AnalogItemType_LimitLo = 196;

        /// <summary>
        /// The identifier for the AnalogItemType_LimitHi Variable.
        /// </summary>
        public const uint AnalogItemType_LimitHi = 197;

        /// <summary>
        /// The identifier for the AnalogItemType_LimitLoLo Variable.
        /// </summary>
        public const uint AnalogItemType_LimitLoLo = 198;

        /// <summary>
        /// The identifier for the AnalogItemType_LimitHiHi Variable.
        /// </summary>
        public const uint AnalogItemType_LimitHiHi = 199;

        /// <summary>
        /// The identifier for the AnalogItemType_EngineeringUnits Variable.
        /// </summary>
        public const uint AnalogItemType_EngineeringUnits = 200;

        /// <summary>
        /// The identifier for the TsFormatType_OpenLabel Variable.
        /// </summary>
        public const uint TsFormatType_OpenLabel = 81;

        /// <summary>
        /// The identifier for the TsFormatType_CloseLabel Variable.
        /// </summary>
        public const uint TsFormatType_CloseLabel = 82;

        /// <summary>
        /// The identifier for the TsFormatType_OpenColor Variable.
        /// </summary>
        public const uint TsFormatType_OpenColor = 83;

        /// <summary>
        /// The identifier for the TsFormatType_CloseColor Variable.
        /// </summary>
        public const uint TsFormatType_CloseColor = 84;

        /// <summary>
        /// The identifier for the SimulationSignalType_Type Variable.
        /// </summary>
        public const uint SimulationSignalType_Type = 229;

        /// <summary>
        /// The identifier for the SimulationSignalType_Period Variable.
        /// </summary>
        public const uint SimulationSignalType_Period = 230;

        /// <summary>
        /// The identifier for the SimulationSignalType_Phase Variable.
        /// </summary>
        public const uint SimulationSignalType_Phase = 231;

        /// <summary>
        /// The identifier for the SimulationSignalType_UpdateInterval Variable.
        /// </summary>
        public const uint SimulationSignalType_UpdateInterval = 232;

        /// <summary>
        /// The identifier for the UserType_AccessRights Variable.
        /// </summary>
        public const uint UserType_AccessRights = 17;

        /// <summary>
        /// The identifier for the RootUser_AccessRights Variable.
        /// </summary>
        public const uint RootUser_AccessRights = 39;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_Depth Variable.
        /// </summary>
        public const uint HistoricalDatabaseType_Depth = 36;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_WriteValueDuration Variable.
        /// </summary>
        public const uint HistoricalDatabaseType_WriteValueDuration = 49;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_PendingTaskCount Variable.
        /// </summary>
        public const uint HistoricalDatabaseType_PendingTaskCount = 50;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_EventCleanupDuration Variable.
        /// </summary>
        public const uint HistoricalDatabaseType_EventCleanupDuration = 51;

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_ValueCleanupDuration Variable.
        /// </summary>
        public const uint HistoricalDatabaseType_ValueCleanupDuration = 52;

        /// <summary>
        /// The identifier for the SystemDatabase_Depth Variable.
        /// </summary>
        public const uint SystemDatabase_Depth = 37;

        /// <summary>
        /// The identifier for the SystemDatabase_WriteValueDuration Variable.
        /// </summary>
        public const uint SystemDatabase_WriteValueDuration = 53;

        /// <summary>
        /// The identifier for the SystemDatabase_PendingTaskCount Variable.
        /// </summary>
        public const uint SystemDatabase_PendingTaskCount = 54;

        /// <summary>
        /// The identifier for the SystemDatabase_EventCleanupDuration Variable.
        /// </summary>
        public const uint SystemDatabase_EventCleanupDuration = 55;

        /// <summary>
        /// The identifier for the SystemDatabase_ValueCleanupDuration Variable.
        /// </summary>
        public const uint SystemDatabase_ValueCleanupDuration = 56;

        /// <summary>
        /// The identifier for the DeviceType_Disabled Variable.
        /// </summary>
        public const uint DeviceType_Disabled = 201;

        /// <summary>
        /// The identifier for the DeviceType_Online Variable.
        /// </summary>
        public const uint DeviceType_Online = 202;

        /// <summary>
        /// The identifier for the DeviceType_Enabled Variable.
        /// </summary>
        public const uint DeviceType_Enabled = 203;

        /// <summary>
        /// The identifier for the LinkType_Transport Variable.
        /// </summary>
        public const uint LinkType_Transport = 42;

        /// <summary>
        /// The identifier for the LinkType_ConnectCount Variable.
        /// </summary>
        public const uint LinkType_ConnectCount = 43;

        /// <summary>
        /// The identifier for the LinkType_ActiveConnections Variable.
        /// </summary>
        public const uint LinkType_ActiveConnections = 44;

        /// <summary>
        /// The identifier for the LinkType_MessagesOut Variable.
        /// </summary>
        public const uint LinkType_MessagesOut = 45;

        /// <summary>
        /// The identifier for the LinkType_MessagesIn Variable.
        /// </summary>
        public const uint LinkType_MessagesIn = 46;

        /// <summary>
        /// The identifier for the LinkType_BytesOut Variable.
        /// </summary>
        public const uint LinkType_BytesOut = 47;

        /// <summary>
        /// The identifier for the LinkType_BytesIn Variable.
        /// </summary>
        public const uint LinkType_BytesIn = 48;

        /// <summary>
        /// The identifier for the Iec60870LinkType_Protocol Variable.
        /// </summary>
        public const uint Iec60870LinkType_Protocol = 314;

        /// <summary>
        /// The identifier for the Iec60870LinkType_Mode Variable.
        /// </summary>
        public const uint Iec60870LinkType_Mode = 315;

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendQueueSize Variable.
        /// </summary>
        public const uint Iec60870LinkType_SendQueueSize = 316;

        /// <summary>
        /// The identifier for the Iec60870LinkType_ReceiveQueueSize Variable.
        /// </summary>
        public const uint Iec60870LinkType_ReceiveQueueSize = 317;

        /// <summary>
        /// The identifier for the Iec60870LinkType_ConnectTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_ConnectTimeout = 318;

        /// <summary>
        /// The identifier for the Iec60870LinkType_ConfirmationTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_ConfirmationTimeout = 319;

        /// <summary>
        /// The identifier for the Iec60870LinkType_TerminationTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_TerminationTimeout = 320;

        /// <summary>
        /// The identifier for the Iec60870LinkType_DeviceAddressSize Variable.
        /// </summary>
        public const uint Iec60870LinkType_DeviceAddressSize = 321;

        /// <summary>
        /// The identifier for the Iec60870LinkType_COTSize Variable.
        /// </summary>
        public const uint Iec60870LinkType_COTSize = 322;

        /// <summary>
        /// The identifier for the Iec60870LinkType_InfoAddressSize Variable.
        /// </summary>
        public const uint Iec60870LinkType_InfoAddressSize = 323;

        /// <summary>
        /// The identifier for the Iec60870LinkType_DataCollection Variable.
        /// </summary>
        public const uint Iec60870LinkType_DataCollection = 324;

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendRetryCount Variable.
        /// </summary>
        public const uint Iec60870LinkType_SendRetryCount = 325;

        /// <summary>
        /// The identifier for the Iec60870LinkType_CRCProtection Variable.
        /// </summary>
        public const uint Iec60870LinkType_CRCProtection = 326;

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_SendTimeout = 327;

        /// <summary>
        /// The identifier for the Iec60870LinkType_ReceiveTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_ReceiveTimeout = 328;

        /// <summary>
        /// The identifier for the Iec60870LinkType_IdleTimeout Variable.
        /// </summary>
        public const uint Iec60870LinkType_IdleTimeout = 329;

        /// <summary>
        /// The identifier for the Iec60870LinkType_AnonymousMode Variable.
        /// </summary>
        public const uint Iec60870LinkType_AnonymousMode = 330;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_Address Variable.
        /// </summary>
        public const uint Iec60870DeviceType_Address = 341;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_LinkAddress Variable.
        /// </summary>
        public const uint Iec60870DeviceType_LinkAddress = 342;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_StartupInterrogation Variable.
        /// </summary>
        public const uint Iec60870DeviceType_StartupInterrogation = 343;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriod Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriod = 344;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_StartupClockSync Variable.
        /// </summary>
        public const uint Iec60870DeviceType_StartupClockSync = 345;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_ClockSyncPeriod Variable.
        /// </summary>
        public const uint Iec60870DeviceType_ClockSyncPeriod = 346;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup1 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup1 = 347;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup2 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup2 = 348;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup3 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup3 = 349;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup4 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup4 = 350;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup5 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup5 = 351;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup6 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup6 = 352;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup7 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup7 = 353;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup8 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup8 = 354;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup9 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup9 = 355;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup10 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup10 = 356;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup11 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup11 = 357;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup12 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup12 = 358;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup13 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup13 = 359;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup14 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup14 = 360;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup15 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup15 = 361;

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup16 Variable.
        /// </summary>
        public const uint Iec60870DeviceType_InterrogationPeriodGroup16 = 362;

        /// <summary>
        /// The identifier for the ModbusLinkModeType_EnumValues Variable.
        /// </summary>
        public const uint ModbusLinkModeType_EnumValues = 97;

        /// <summary>
        /// The identifier for the ModbusLinkType_Mode Variable.
        /// </summary>
        public const uint ModbusLinkType_Mode = 117;

        /// <summary>
        /// The identifier for the ModbusDeviceType_Address Variable.
        /// </summary>
        public const uint ModbusDeviceType_Address = 120;

        /// <summary>
        /// The identifier for the ModbusDeviceType_SendRetryCount Variable.
        /// </summary>
        public const uint ModbusDeviceType_SendRetryCount = 121;

        /// <summary>
        /// The identifier for the TransmissionItemType_SourceAddress Variable.
        /// </summary>
        public const uint TransmissionItemType_SourceAddress = 35;

        /// <summary>
        /// The identifier for the Scada_XmlSchema Variable.
        /// </summary>
        public const uint Scada_XmlSchema = 98;

        /// <summary>
        /// The identifier for the Scada_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint Scada_XmlSchema_NamespaceUri = 100;

        /// <summary>
        /// The identifier for the Scada_BinarySchema Variable.
        /// </summary>
        public const uint Scada_BinarySchema = 101;

        /// <summary>
        /// The identifier for the Scada_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint Scada_BinarySchema_NamespaceUri = 103;
    }
    #endregion

    #region VariableType Identifiers
    /// <summary>
    /// A class that declares constants for all VariableTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableTypes
    {
        /// <summary>
        /// The identifier for the DataItemType VariableType.
        /// </summary>
        public const uint DataItemType = 18;

        /// <summary>
        /// The identifier for the DiscreteItemType VariableType.
        /// </summary>
        public const uint DiscreteItemType = 72;

        /// <summary>
        /// The identifier for the AnalogItemType VariableType.
        /// </summary>
        public const uint AnalogItemType = 76;

        /// <summary>
        /// The identifier for the SimulationSignalType VariableType.
        /// </summary>
        public const uint SimulationSignalType = 67;
    }
    #endregion

    #region DataType Node Identifiers
    /// <summary>
    /// A class that declares constants for all DataTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class DataTypeIds
    {
        /// <summary>
        /// The identifier for the ModbusLinkModeType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ModbusLinkModeType = new ExpandedNodeId(Telecontrol.Scada.DataTypes.ModbusLinkModeType, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region Method Node Identifiers
    /// <summary>
    /// A class that declares constants for all Methods in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class MethodIds
    {
        /// <summary>
        /// The identifier for the DataItemType_WriteManual Method.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_WriteManual = new ExpandedNodeId(Telecontrol.Scada.Methods.DataItemType_WriteManual, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Interrogate Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Interrogate = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_Interrogate, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_InterrogateChannel Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_InterrogateChannel = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_InterrogateChannel, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_SyncClock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_SyncClock = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_SyncClock, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Select Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Select = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_Select, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Write = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_Write, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_WriteParam Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_WriteParam = new ExpandedNodeId(Telecontrol.Scada.Methods.DeviceType_WriteParam, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region Object Node Identifiers
    /// <summary>
    /// A class that declares constants for all Objects in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectIds
    {
        /// <summary>
        /// The identifier for the DataItems Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItems = new ExpandedNodeId(Telecontrol.Scada.Objects.DataItems, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormats Object.
        /// </summary>
        public static readonly ExpandedNodeId TsFormats = new ExpandedNodeId(Telecontrol.Scada.Objects.TsFormats, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignals Object.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignals = new ExpandedNodeId(Telecontrol.Scada.Objects.SimulationSignals, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Users Object.
        /// </summary>
        public static readonly ExpandedNodeId Users = new ExpandedNodeId(Telecontrol.Scada.Objects.Users, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the RootUser Object.
        /// </summary>
        public static readonly ExpandedNodeId RootUser = new ExpandedNodeId(Telecontrol.Scada.Objects.RootUser, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabases Object.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabases = new ExpandedNodeId(Telecontrol.Scada.Objects.HistoricalDatabases, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase Object.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase = new ExpandedNodeId(Telecontrol.Scada.Objects.SystemDatabase, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Devices Object.
        /// </summary>
        public static readonly ExpandedNodeId Devices = new ExpandedNodeId(Telecontrol.Scada.Objects.Devices, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TransmissionItems Object.
        /// </summary>
        public static readonly ExpandedNodeId TransmissionItems = new ExpandedNodeId(Telecontrol.Scada.Objects.TransmissionItems, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region ObjectType Node Identifiers
    /// <summary>
    /// A class that declares constants for all ObjectTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectTypeIds
    {
        /// <summary>
        /// The identifier for the DataGroupType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DataGroupType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.DataGroupType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormatType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TsFormatType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.TsFormatType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the UserType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId UserType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.UserType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.HistoricalDatabaseType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.DeviceType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId LinkType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.LinkType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceWatchEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DeviceWatchEventType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.DeviceWatchEventType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.Iec60870LinkType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.Iec60870DeviceType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusLinkType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ModbusLinkType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.ModbusLinkType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ModbusDeviceType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.ModbusDeviceType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TransmissionItemType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TransmissionItemType = new ExpandedNodeId(Telecontrol.Scada.ObjectTypes.TransmissionItemType, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region ReferenceType Node Identifiers
    /// <summary>
    /// A class that declares constants for all ReferenceTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ReferenceTypeIds
    {
        /// <summary>
        /// The identifier for the Creates ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId Creates = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.Creates, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HasSimulationSignal ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasSimulationSignal = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.HasSimulationSignal, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HasHistoricalDatabase ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasHistoricalDatabase = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.HasHistoricalDatabase, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HasTsFormat ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasTsFormat = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.HasTsFormat, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HasTransmissionSource ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasTransmissionSource = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.HasTransmissionSource, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HasTransmissionTarget ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasTransmissionTarget = new ExpandedNodeId(Telecontrol.Scada.ReferenceTypes.HasTransmissionTarget, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region Variable Node Identifiers
    /// <summary>
    /// A class that declares constants for all Variables in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableIds
    {
        /// <summary>
        /// The identifier for the DataGroupType_Simulated Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataGroupType_Simulated = new ExpandedNodeId(Telecontrol.Scada.Variables.DataGroupType_Simulated, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Alias Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Alias = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Alias, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Severity Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Severity = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Severity, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Input1 Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Input1 = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Input1, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Input2 Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Input2 = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Input2, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Output Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Output = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Output, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_OutputCondition Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_OutputCondition = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_OutputCondition, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_StalePeriod Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_StalePeriod = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_StalePeriod, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Simulated Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Simulated = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Simulated, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DataItemType_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType_Locked = new ExpandedNodeId(Telecontrol.Scada.Variables.DataItemType_Locked, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DiscreteItemType_Inversion Variable.
        /// </summary>
        public static readonly ExpandedNodeId DiscreteItemType_Inversion = new ExpandedNodeId(Telecontrol.Scada.Variables.DiscreteItemType_Inversion, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_DisplayFormat Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_DisplayFormat = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_DisplayFormat, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_Conversion Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_Conversion = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_Conversion, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_Clamping Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_Clamping = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_Clamping, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_EuLo Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_EuLo = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_EuLo, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_EuHi Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_EuHi = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_EuHi, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_IrLo Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_IrLo = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_IrLo, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_IrHi Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_IrHi = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_IrHi, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_LimitLo Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_LimitLo = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_LimitLo, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_LimitHi Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_LimitHi = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_LimitHi, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_LimitLoLo Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_LimitLoLo = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_LimitLoLo, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_LimitHiHi Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_LimitHiHi = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_LimitHiHi, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType_EngineeringUnits = new ExpandedNodeId(Telecontrol.Scada.Variables.AnalogItemType_EngineeringUnits, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormatType_OpenLabel Variable.
        /// </summary>
        public static readonly ExpandedNodeId TsFormatType_OpenLabel = new ExpandedNodeId(Telecontrol.Scada.Variables.TsFormatType_OpenLabel, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormatType_CloseLabel Variable.
        /// </summary>
        public static readonly ExpandedNodeId TsFormatType_CloseLabel = new ExpandedNodeId(Telecontrol.Scada.Variables.TsFormatType_CloseLabel, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormatType_OpenColor Variable.
        /// </summary>
        public static readonly ExpandedNodeId TsFormatType_OpenColor = new ExpandedNodeId(Telecontrol.Scada.Variables.TsFormatType_OpenColor, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TsFormatType_CloseColor Variable.
        /// </summary>
        public static readonly ExpandedNodeId TsFormatType_CloseColor = new ExpandedNodeId(Telecontrol.Scada.Variables.TsFormatType_CloseColor, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignalType_Type Variable.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignalType_Type = new ExpandedNodeId(Telecontrol.Scada.Variables.SimulationSignalType_Type, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignalType_Period Variable.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignalType_Period = new ExpandedNodeId(Telecontrol.Scada.Variables.SimulationSignalType_Period, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignalType_Phase Variable.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignalType_Phase = new ExpandedNodeId(Telecontrol.Scada.Variables.SimulationSignalType_Phase, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignalType_UpdateInterval Variable.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignalType_UpdateInterval = new ExpandedNodeId(Telecontrol.Scada.Variables.SimulationSignalType_UpdateInterval, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the UserType_AccessRights Variable.
        /// </summary>
        public static readonly ExpandedNodeId UserType_AccessRights = new ExpandedNodeId(Telecontrol.Scada.Variables.UserType_AccessRights, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the RootUser_AccessRights Variable.
        /// </summary>
        public static readonly ExpandedNodeId RootUser_AccessRights = new ExpandedNodeId(Telecontrol.Scada.Variables.RootUser_AccessRights, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_Depth Variable.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType_Depth = new ExpandedNodeId(Telecontrol.Scada.Variables.HistoricalDatabaseType_Depth, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_WriteValueDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType_WriteValueDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.HistoricalDatabaseType_WriteValueDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_PendingTaskCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType_PendingTaskCount = new ExpandedNodeId(Telecontrol.Scada.Variables.HistoricalDatabaseType_PendingTaskCount, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_EventCleanupDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType_EventCleanupDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.HistoricalDatabaseType_EventCleanupDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the HistoricalDatabaseType_ValueCleanupDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId HistoricalDatabaseType_ValueCleanupDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.HistoricalDatabaseType_ValueCleanupDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase_Depth Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase_Depth = new ExpandedNodeId(Telecontrol.Scada.Variables.SystemDatabase_Depth, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase_WriteValueDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase_WriteValueDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.SystemDatabase_WriteValueDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase_PendingTaskCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase_PendingTaskCount = new ExpandedNodeId(Telecontrol.Scada.Variables.SystemDatabase_PendingTaskCount, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase_EventCleanupDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase_EventCleanupDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.SystemDatabase_EventCleanupDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SystemDatabase_ValueCleanupDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemDatabase_ValueCleanupDuration = new ExpandedNodeId(Telecontrol.Scada.Variables.SystemDatabase_ValueCleanupDuration, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Disabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Disabled = new ExpandedNodeId(Telecontrol.Scada.Variables.DeviceType_Disabled, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Online Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Online = new ExpandedNodeId(Telecontrol.Scada.Variables.DeviceType_Online, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DeviceType_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Enabled = new ExpandedNodeId(Telecontrol.Scada.Variables.DeviceType_Enabled, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_Transport Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_Transport = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_Transport, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_ConnectCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_ConnectCount = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_ConnectCount, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_ActiveConnections Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_ActiveConnections = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_ActiveConnections, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_MessagesOut Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_MessagesOut = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_MessagesOut, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_MessagesIn Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_MessagesIn = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_MessagesIn, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_BytesOut Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_BytesOut = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_BytesOut, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the LinkType_BytesIn Variable.
        /// </summary>
        public static readonly ExpandedNodeId LinkType_BytesIn = new ExpandedNodeId(Telecontrol.Scada.Variables.LinkType_BytesIn, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_Protocol Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_Protocol = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_Protocol, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_Mode Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_Mode = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_Mode, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendQueueSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_SendQueueSize = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_SendQueueSize, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_ReceiveQueueSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_ReceiveQueueSize = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_ReceiveQueueSize, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_ConnectTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_ConnectTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_ConnectTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_ConfirmationTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_ConfirmationTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_ConfirmationTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_TerminationTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_TerminationTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_TerminationTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_DeviceAddressSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_DeviceAddressSize = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_DeviceAddressSize, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_COTSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_COTSize = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_COTSize, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_InfoAddressSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_InfoAddressSize = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_InfoAddressSize, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_DataCollection Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_DataCollection = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_DataCollection, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendRetryCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_SendRetryCount = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_SendRetryCount, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_CRCProtection Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_CRCProtection = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_CRCProtection, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_SendTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_SendTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_SendTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_ReceiveTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_ReceiveTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_ReceiveTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_IdleTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_IdleTimeout = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_IdleTimeout, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870LinkType_AnonymousMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870LinkType_AnonymousMode = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870LinkType_AnonymousMode, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_Address = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_Address, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_LinkAddress Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_LinkAddress = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_LinkAddress, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_StartupInterrogation Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_StartupInterrogation = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_StartupInterrogation, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriod Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriod = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriod, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_StartupClockSync Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_StartupClockSync = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_StartupClockSync, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_ClockSyncPeriod Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_ClockSyncPeriod = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_ClockSyncPeriod, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup1 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup1 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup1, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup2 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup2 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup2, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup3 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup3 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup3, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup4 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup4 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup4, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup5 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup5 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup5, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup6 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup6 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup6, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup7 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup7 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup7, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup8 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup8 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup8, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup9 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup9 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup9, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup10 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup10 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup10, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup11 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup11 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup11, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup12 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup12 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup12, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup13 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup13 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup13, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup14 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup14 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup14, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup15 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup15 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup15, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Iec60870DeviceType_InterrogationPeriodGroup16 Variable.
        /// </summary>
        public static readonly ExpandedNodeId Iec60870DeviceType_InterrogationPeriodGroup16 = new ExpandedNodeId(Telecontrol.Scada.Variables.Iec60870DeviceType_InterrogationPeriodGroup16, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusLinkModeType_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ModbusLinkModeType_EnumValues = new ExpandedNodeId(Telecontrol.Scada.Variables.ModbusLinkModeType_EnumValues, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusLinkType_Mode Variable.
        /// </summary>
        public static readonly ExpandedNodeId ModbusLinkType_Mode = new ExpandedNodeId(Telecontrol.Scada.Variables.ModbusLinkType_Mode, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusDeviceType_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId ModbusDeviceType_Address = new ExpandedNodeId(Telecontrol.Scada.Variables.ModbusDeviceType_Address, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the ModbusDeviceType_SendRetryCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId ModbusDeviceType_SendRetryCount = new ExpandedNodeId(Telecontrol.Scada.Variables.ModbusDeviceType_SendRetryCount, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the TransmissionItemType_SourceAddress Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransmissionItemType_SourceAddress = new ExpandedNodeId(Telecontrol.Scada.Variables.TransmissionItemType_SourceAddress, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Scada_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId Scada_XmlSchema = new ExpandedNodeId(Telecontrol.Scada.Variables.Scada_XmlSchema, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Scada_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId Scada_XmlSchema_NamespaceUri = new ExpandedNodeId(Telecontrol.Scada.Variables.Scada_XmlSchema_NamespaceUri, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Scada_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId Scada_BinarySchema = new ExpandedNodeId(Telecontrol.Scada.Variables.Scada_BinarySchema, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the Scada_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId Scada_BinarySchema_NamespaceUri = new ExpandedNodeId(Telecontrol.Scada.Variables.Scada_BinarySchema_NamespaceUri, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region VariableType Node Identifiers
    /// <summary>
    /// A class that declares constants for all VariableTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableTypeIds
    {
        /// <summary>
        /// The identifier for the DataItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemType = new ExpandedNodeId(Telecontrol.Scada.VariableTypes.DataItemType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the DiscreteItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId DiscreteItemType = new ExpandedNodeId(Telecontrol.Scada.VariableTypes.DiscreteItemType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the AnalogItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AnalogItemType = new ExpandedNodeId(Telecontrol.Scada.VariableTypes.AnalogItemType, Telecontrol.Scada.Namespaces.Scada);

        /// <summary>
        /// The identifier for the SimulationSignalType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId SimulationSignalType = new ExpandedNodeId(Telecontrol.Scada.VariableTypes.SimulationSignalType, Telecontrol.Scada.Namespaces.Scada);
    }
    #endregion

    #region BrowseName Declarations
    /// <summary>
    /// Declares all of the BrowseNames used in the Model Design.
    /// </summary>
    public static partial class BrowseNames
    {
        /// <summary>
        /// The BrowseName for the AccessRights component.
        /// </summary>
        public const string AccessRights = "AccessRights";

        /// <summary>
        /// The BrowseName for the ActiveConnections component.
        /// </summary>
        public const string ActiveConnections = "ActiveConnections";

        /// <summary>
        /// The BrowseName for the Address component.
        /// </summary>
        public const string Address = "Address";

        /// <summary>
        /// The BrowseName for the Alias component.
        /// </summary>
        public const string Alias = "Alias";

        /// <summary>
        /// The BrowseName for the AnalogItemType component.
        /// </summary>
        public const string AnalogItemType = "AnalogItemType";

        /// <summary>
        /// The BrowseName for the AnonymousMode component.
        /// </summary>
        public const string AnonymousMode = "AnonymousMode";

        /// <summary>
        /// The BrowseName for the BytesIn component.
        /// </summary>
        public const string BytesIn = "BytesIn";

        /// <summary>
        /// The BrowseName for the BytesOut component.
        /// </summary>
        public const string BytesOut = "BytesOut";

        /// <summary>
        /// The BrowseName for the Clamping component.
        /// </summary>
        public const string Clamping = "Clamping";

        /// <summary>
        /// The BrowseName for the ClockSyncPeriod component.
        /// </summary>
        public const string ClockSyncPeriod = "ClockSyncPeriod";

        /// <summary>
        /// The BrowseName for the CloseColor component.
        /// </summary>
        public const string CloseColor = "CloseColor";

        /// <summary>
        /// The BrowseName for the CloseLabel component.
        /// </summary>
        public const string CloseLabel = "CloseLabel";

        /// <summary>
        /// The BrowseName for the ConfirmationTimeout component.
        /// </summary>
        public const string ConfirmationTimeout = "ConfirmationTimeout";

        /// <summary>
        /// The BrowseName for the ConnectCount component.
        /// </summary>
        public const string ConnectCount = "ConnectCount";

        /// <summary>
        /// The BrowseName for the ConnectTimeout component.
        /// </summary>
        public const string ConnectTimeout = "ConnectTimeout";

        /// <summary>
        /// The BrowseName for the Conversion component.
        /// </summary>
        public const string Conversion = "Conversion";

        /// <summary>
        /// The BrowseName for the COTSize component.
        /// </summary>
        public const string COTSize = "COTSize";

        /// <summary>
        /// The BrowseName for the CRCProtection component.
        /// </summary>
        public const string CRCProtection = "CRCProtection";

        /// <summary>
        /// The BrowseName for the Creates component.
        /// </summary>
        public const string Creates = "Creates";

        /// <summary>
        /// The BrowseName for the DataCollection component.
        /// </summary>
        public const string DataCollection = "DataCollection";

        /// <summary>
        /// The BrowseName for the DataGroupType component.
        /// </summary>
        public const string DataGroupType = "DataGroupType";

        /// <summary>
        /// The BrowseName for the DataItems component.
        /// </summary>
        public const string DataItems = "DataItems";

        /// <summary>
        /// The BrowseName for the DataItemType component.
        /// </summary>
        public const string DataItemType = "DataItemType";

        /// <summary>
        /// The BrowseName for the Depth component.
        /// </summary>
        public const string Depth = "Depth";

        /// <summary>
        /// The BrowseName for the DeviceAddressSize component.
        /// </summary>
        public const string DeviceAddressSize = "DeviceAddressSize";

        /// <summary>
        /// The BrowseName for the Devices component.
        /// </summary>
        public const string Devices = "Devices";

        /// <summary>
        /// The BrowseName for the DeviceType component.
        /// </summary>
        public const string DeviceType = "DeviceType";

        /// <summary>
        /// The BrowseName for the DeviceWatchEventType component.
        /// </summary>
        public const string DeviceWatchEventType = "DeviceWatchEventType";

        /// <summary>
        /// The BrowseName for the Disabled component.
        /// </summary>
        public const string Disabled = "Disabled";

        /// <summary>
        /// The BrowseName for the DiscreteItemType component.
        /// </summary>
        public const string DiscreteItemType = "DiscreteItemType";

        /// <summary>
        /// The BrowseName for the DisplayFormat component.
        /// </summary>
        public const string DisplayFormat = "DisplayFormat";

        /// <summary>
        /// The BrowseName for the Enabled component.
        /// </summary>
        public const string Enabled = "Enabled";

        /// <summary>
        /// The BrowseName for the EngineeringUnits component.
        /// </summary>
        public const string EngineeringUnits = "EngineeringUnits";

        /// <summary>
        /// The BrowseName for the EuHi component.
        /// </summary>
        public const string EuHi = "EuHi";

        /// <summary>
        /// The BrowseName for the EuLo component.
        /// </summary>
        public const string EuLo = "EuLo";

        /// <summary>
        /// The BrowseName for the EventCleanupDuration component.
        /// </summary>
        public const string EventCleanupDuration = "EventCleanupDuration";

        /// <summary>
        /// The BrowseName for the HasHistoricalDatabase component.
        /// </summary>
        public const string HasHistoricalDatabase = "HasHistoricalDatabase";

        /// <summary>
        /// The BrowseName for the HasSimulationSignal component.
        /// </summary>
        public const string HasSimulationSignal = "HasSimulationSignal";

        /// <summary>
        /// The BrowseName for the HasTransmissionSource component.
        /// </summary>
        public const string HasTransmissionSource = "HasTransmissionSource";

        /// <summary>
        /// The BrowseName for the HasTransmissionTarget component.
        /// </summary>
        public const string HasTransmissionTarget = "HasTransmissionTarget";

        /// <summary>
        /// The BrowseName for the HasTsFormat component.
        /// </summary>
        public const string HasTsFormat = "HasTsFormat";

        /// <summary>
        /// The BrowseName for the HistoricalDatabases component.
        /// </summary>
        public const string HistoricalDatabases = "HistoricalDatabases";

        /// <summary>
        /// The BrowseName for the HistoricalDatabaseType component.
        /// </summary>
        public const string HistoricalDatabaseType = "HistoricalDatabaseType";

        /// <summary>
        /// The BrowseName for the IdleTimeout component.
        /// </summary>
        public const string IdleTimeout = "IdleTimeout";

        /// <summary>
        /// The BrowseName for the Iec60870DeviceType component.
        /// </summary>
        public const string Iec60870DeviceType = "Iec60870DeviceType";

        /// <summary>
        /// The BrowseName for the Iec60870LinkType component.
        /// </summary>
        public const string Iec60870LinkType = "Iec60870LinkType";

        /// <summary>
        /// The BrowseName for the InfoAddressSize component.
        /// </summary>
        public const string InfoAddressSize = "InfoAddressSize";

        /// <summary>
        /// The BrowseName for the Input1 component.
        /// </summary>
        public const string Input1 = "Input1";

        /// <summary>
        /// The BrowseName for the Input2 component.
        /// </summary>
        public const string Input2 = "Input2";

        /// <summary>
        /// The BrowseName for the Interrogate component.
        /// </summary>
        public const string Interrogate = "Interrogate";

        /// <summary>
        /// The BrowseName for the InterrogateChannel component.
        /// </summary>
        public const string InterrogateChannel = "InterrogateChannel";

        /// <summary>
        /// The BrowseName for the InterrogationPeriod component.
        /// </summary>
        public const string InterrogationPeriod = "InterrogationPeriod";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup1 component.
        /// </summary>
        public const string InterrogationPeriodGroup1 = "InterrogationPeriodGroup1";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup10 component.
        /// </summary>
        public const string InterrogationPeriodGroup10 = "InterrogationPeriodGroup10";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup11 component.
        /// </summary>
        public const string InterrogationPeriodGroup11 = "InterrogationPeriodGroup11";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup12 component.
        /// </summary>
        public const string InterrogationPeriodGroup12 = "InterrogationPeriodGroup12";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup13 component.
        /// </summary>
        public const string InterrogationPeriodGroup13 = "InterrogationPeriodGroup13";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup14 component.
        /// </summary>
        public const string InterrogationPeriodGroup14 = "InterrogationPeriodGroup14";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup15 component.
        /// </summary>
        public const string InterrogationPeriodGroup15 = "InterrogationPeriodGroup15";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup16 component.
        /// </summary>
        public const string InterrogationPeriodGroup16 = "InterrogationPeriodGroup16";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup2 component.
        /// </summary>
        public const string InterrogationPeriodGroup2 = "InterrogationPeriodGroup2";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup3 component.
        /// </summary>
        public const string InterrogationPeriodGroup3 = "InterrogationPeriodGroup3";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup4 component.
        /// </summary>
        public const string InterrogationPeriodGroup4 = "InterrogationPeriodGroup4";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup5 component.
        /// </summary>
        public const string InterrogationPeriodGroup5 = "InterrogationPeriodGroup5";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup6 component.
        /// </summary>
        public const string InterrogationPeriodGroup6 = "InterrogationPeriodGroup6";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup7 component.
        /// </summary>
        public const string InterrogationPeriodGroup7 = "InterrogationPeriodGroup7";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup8 component.
        /// </summary>
        public const string InterrogationPeriodGroup8 = "InterrogationPeriodGroup8";

        /// <summary>
        /// The BrowseName for the InterrogationPeriodGroup9 component.
        /// </summary>
        public const string InterrogationPeriodGroup9 = "InterrogationPeriodGroup9";

        /// <summary>
        /// The BrowseName for the Inversion component.
        /// </summary>
        public const string Inversion = "Inversion";

        /// <summary>
        /// The BrowseName for the IrHi component.
        /// </summary>
        public const string IrHi = "IrHi";

        /// <summary>
        /// The BrowseName for the IrLo component.
        /// </summary>
        public const string IrLo = "IrLo";

        /// <summary>
        /// The BrowseName for the LimitHi component.
        /// </summary>
        public const string LimitHi = "LimitHi";

        /// <summary>
        /// The BrowseName for the LimitHiHi component.
        /// </summary>
        public const string LimitHiHi = "LimitHiHi";

        /// <summary>
        /// The BrowseName for the LimitLo component.
        /// </summary>
        public const string LimitLo = "LimitLo";

        /// <summary>
        /// The BrowseName for the LimitLoLo component.
        /// </summary>
        public const string LimitLoLo = "LimitLoLo";

        /// <summary>
        /// The BrowseName for the LinkAddress component.
        /// </summary>
        public const string LinkAddress = "LinkAddress";

        /// <summary>
        /// The BrowseName for the LinkType component.
        /// </summary>
        public const string LinkType = "LinkType";

        /// <summary>
        /// The BrowseName for the Locked component.
        /// </summary>
        public const string Locked = "Locked";

        /// <summary>
        /// The BrowseName for the MessagesIn component.
        /// </summary>
        public const string MessagesIn = "MessagesIn";

        /// <summary>
        /// The BrowseName for the MessagesOut component.
        /// </summary>
        public const string MessagesOut = "MessagesOut";

        /// <summary>
        /// The BrowseName for the ModbusDeviceType component.
        /// </summary>
        public const string ModbusDeviceType = "ModbusDeviceType";

        /// <summary>
        /// The BrowseName for the ModbusLinkModeType component.
        /// </summary>
        public const string ModbusLinkModeType = "ModbusLinkModeType";

        /// <summary>
        /// The BrowseName for the ModbusLinkType component.
        /// </summary>
        public const string ModbusLinkType = "ModbusLinkType";

        /// <summary>
        /// The BrowseName for the Mode component.
        /// </summary>
        public const string Mode = "Mode";

        /// <summary>
        /// The BrowseName for the Online component.
        /// </summary>
        public const string Online = "Online";

        /// <summary>
        /// The BrowseName for the OpenColor component.
        /// </summary>
        public const string OpenColor = "OpenColor";

        /// <summary>
        /// The BrowseName for the OpenLabel component.
        /// </summary>
        public const string OpenLabel = "OpenLabel";

        /// <summary>
        /// The BrowseName for the Output component.
        /// </summary>
        public const string Output = "Output";

        /// <summary>
        /// The BrowseName for the OutputCondition component.
        /// </summary>
        public const string OutputCondition = "OutputCondition";

        /// <summary>
        /// The BrowseName for the PendingTaskCount component.
        /// </summary>
        public const string PendingTaskCount = "PendingTaskCount";

        /// <summary>
        /// The BrowseName for the Period component.
        /// </summary>
        public const string Period = "Period";

        /// <summary>
        /// The BrowseName for the Phase component.
        /// </summary>
        public const string Phase = "Phase";

        /// <summary>
        /// The BrowseName for the Protocol component.
        /// </summary>
        public const string Protocol = "Protocol";

        /// <summary>
        /// The BrowseName for the ReceiveQueueSize component.
        /// </summary>
        public const string ReceiveQueueSize = "ReceiveQueueSize";

        /// <summary>
        /// The BrowseName for the ReceiveTimeout component.
        /// </summary>
        public const string ReceiveTimeout = "ReceiveTimeout";

        /// <summary>
        /// The BrowseName for the RootUser component.
        /// </summary>
        public const string RootUser = "root";

        /// <summary>
        /// The BrowseName for the Scada_BinarySchema component.
        /// </summary>
        public const string Scada_BinarySchema = "Telecontrol.Scada";

        /// <summary>
        /// The BrowseName for the Scada_XmlSchema component.
        /// </summary>
        public const string Scada_XmlSchema = "Telecontrol.Scada";

        /// <summary>
        /// The BrowseName for the Select component.
        /// </summary>
        public const string Select = "Select";

        /// <summary>
        /// The BrowseName for the SendQueueSize component.
        /// </summary>
        public const string SendQueueSize = "SendQueueSize";

        /// <summary>
        /// The BrowseName for the SendRetryCount component.
        /// </summary>
        public const string SendRetryCount = "SendRetryCount";

        /// <summary>
        /// The BrowseName for the SendTimeout component.
        /// </summary>
        public const string SendTimeout = "SendTimeout";

        /// <summary>
        /// The BrowseName for the Severity component.
        /// </summary>
        public const string Severity = "Severity";

        /// <summary>
        /// The BrowseName for the Simulated component.
        /// </summary>
        public const string Simulated = "Simulated";

        /// <summary>
        /// The BrowseName for the SimulationSignals component.
        /// </summary>
        public const string SimulationSignals = "SimulationSignals";

        /// <summary>
        /// The BrowseName for the SimulationSignalType component.
        /// </summary>
        public const string SimulationSignalType = "SimulationSignalType";

        /// <summary>
        /// The BrowseName for the SourceAddress component.
        /// </summary>
        public const string SourceAddress = "SourceAddress";

        /// <summary>
        /// The BrowseName for the StalePeriod component.
        /// </summary>
        public const string StalePeriod = "StalePeriod";

        /// <summary>
        /// The BrowseName for the StartupClockSync component.
        /// </summary>
        public const string StartupClockSync = "StartupClockSync";

        /// <summary>
        /// The BrowseName for the StartupInterrogation component.
        /// </summary>
        public const string StartupInterrogation = "StartupInterrogation";

        /// <summary>
        /// The BrowseName for the SyncClock component.
        /// </summary>
        public const string SyncClock = "SyncClock";

        /// <summary>
        /// The BrowseName for the SystemDatabase component.
        /// </summary>
        public const string SystemDatabase = "System";

        /// <summary>
        /// The BrowseName for the TerminationTimeout component.
        /// </summary>
        public const string TerminationTimeout = "TerminationTimeout";

        /// <summary>
        /// The BrowseName for the TransmissionItems component.
        /// </summary>
        public const string TransmissionItems = "TransmissionItems";

        /// <summary>
        /// The BrowseName for the TransmissionItemType component.
        /// </summary>
        public const string TransmissionItemType = "TransmissionItemType";

        /// <summary>
        /// The BrowseName for the Transport component.
        /// </summary>
        public const string Transport = "Transport";

        /// <summary>
        /// The BrowseName for the TsFormats component.
        /// </summary>
        public const string TsFormats = "TsFormats";

        /// <summary>
        /// The BrowseName for the TsFormatType component.
        /// </summary>
        public const string TsFormatType = "TsFormatType";

        /// <summary>
        /// The BrowseName for the Type component.
        /// </summary>
        public const string Type = "Type";

        /// <summary>
        /// The BrowseName for the UpdateInterval component.
        /// </summary>
        public const string UpdateInterval = "UpdateInterval";

        /// <summary>
        /// The BrowseName for the Users component.
        /// </summary>
        public const string Users = "Users";

        /// <summary>
        /// The BrowseName for the UserType component.
        /// </summary>
        public const string UserType = "UserType";

        /// <summary>
        /// The BrowseName for the ValueCleanupDuration component.
        /// </summary>
        public const string ValueCleanupDuration = "ValueCleanupDuration";

        /// <summary>
        /// The BrowseName for the Write component.
        /// </summary>
        public const string Write = "Write";

        /// <summary>
        /// The BrowseName for the WriteManual component.
        /// </summary>
        public const string WriteManual = "WriteManual";

        /// <summary>
        /// The BrowseName for the WriteParam component.
        /// </summary>
        public const string WriteParam = "WriteParam";

        /// <summary>
        /// The BrowseName for the WriteValueDuration component.
        /// </summary>
        public const string WriteValueDuration = "WriteValueDuration";
    }
    #endregion

    #region Namespace Declarations
    /// <summary>
    /// Defines constants for all namespaces referenced by the model design.
    /// </summary>
    public static partial class Namespaces
    {
        /// <summary>
        /// The URI for the OpcUa namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUa = "http://opcfoundation.org/UA/";

        /// <summary>
        /// The URI for the OpcUaXsd namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUaXsd = "http://opcfoundation.org/UA/2008/02/Types.xsd";

        /// <summary>
        /// The URI for the Scada namespace (.NET code namespace is 'Telecontrol.Scada').
        /// </summary>
        public const string Scada = "http://telecontrol.ru/opc-ua/base/";
    }
    #endregion

    #region DataGroupState Class
    #if (!OPCUA_EXCLUDE_DataGroupState)
    /// <summary>
    /// Stores an instance of the DataGroupType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DataGroupState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DataGroupState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.DataGroupType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAVAAAA" +
           "RGF0YUdyb3VwVHlwZUluc3RhbmNlAQE+AAEBPgACAAAAAQEXAAEBAT4AAQEXAAEBARgAAQAAABVgiQoC" +
           "AAAAAQAJAAAAU2ltdWxhdGVkAQE/AAAuAEQ/AAAAAAH/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Simulated Property.
        /// </summary>
        public PropertyState<bool> Simulated
        {
            get
            {
                return m_simulated;
            }

            set
            {
                if (!Object.ReferenceEquals(m_simulated, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_simulated = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_simulated != null)
            {
                children.Add(m_simulated);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Simulated:
                {
                    if (createOrReplace)
                    {
                        if (Simulated == null)
                        {
                            if (replacement == null)
                            {
                                Simulated = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Simulated = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Simulated;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<bool> m_simulated;
        #endregion
    }
    #endif
    #endregion

    #region DataItemState Class
    #if (!OPCUA_EXCLUDE_DataItemState)
    /// <summary>
    /// Stores an instance of the DataItemType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DataItemState : BaseDataVariableState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DataItemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.VariableTypes.DataItemType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.BaseDataType, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////xVggQACAAAAAQAUAAAA" +
           "RGF0YUl0ZW1UeXBlSW5zdGFuY2UBARIAAQESAAAYAQEEAAAAAQFFAAABAUMAAQFGAAABARQAAQEXAAEB" +
           "AT4AAQEXAAEBARgACgAAABVgiQoCAAAAAQAFAAAAQWxpYXMBARMAAC4ARBMAAAAADP////8BAf////8A" +
           "AAAAFWCpCgIAAAABAAgAAABTZXZlcml0eQEBrgAALgBErgAAAAcBAAAAAAf/////AQH/////AAAAABVg" +
           "iQoCAAAAAQAGAAAASW5wdXQxAQGvAAAuAESvAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAGAAAA" +
           "SW5wdXQyAQGwAAAuAESwAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAGAAAAT3V0cHV0AQGxAAAu" +
           "AESxAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAPAAAAT3V0cHV0Q29uZGl0aW9uAQGyAAAuAESy" +
           "AAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQALAAAAU3RhbGVQZXJpb2QBAbMAAC4ARLMAAAAAB///" +
           "//8BAf////8AAAAAFWCJCgIAAAABAAkAAABTaW11bGF0ZWQBAUIAAC4AREIAAAAAAf////8BAf////8A" +
           "AAAAFWCJCgIAAAABAAYAAABMb2NrZWQBAbQAAC4ARLQAAAAAAf////8BAf////8AAAAABGGCCgQAAAAB" +
           "AAsAAABXcml0ZU1hbnVhbAEBHQEALwEBHQEdAQAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Alias Property.
        /// </summary>
        public PropertyState<string> Alias
        {
            get
            {
                return m_alias;
            }

            set
            {
                if (!Object.ReferenceEquals(m_alias, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_alias = value;
            }
        }

        /// <summary>
        /// A description for the Severity Property.
        /// </summary>
        public PropertyState<uint> Severity
        {
            get
            {
                return m_severity;
            }

            set
            {
                if (!Object.ReferenceEquals(m_severity, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_severity = value;
            }
        }

        /// <summary>
        /// A description for the Input1 Property.
        /// </summary>
        public PropertyState<string> Input1
        {
            get
            {
                return m_input1;
            }

            set
            {
                if (!Object.ReferenceEquals(m_input1, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_input1 = value;
            }
        }

        /// <summary>
        /// A description for the Input2 Property.
        /// </summary>
        public PropertyState<string> Input2
        {
            get
            {
                return m_input2;
            }

            set
            {
                if (!Object.ReferenceEquals(m_input2, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_input2 = value;
            }
        }

        /// <summary>
        /// A description for the Output Property.
        /// </summary>
        public PropertyState<string> Output
        {
            get
            {
                return m_output;
            }

            set
            {
                if (!Object.ReferenceEquals(m_output, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_output = value;
            }
        }

        /// <summary>
        /// A description for the OutputCondition Property.
        /// </summary>
        public PropertyState<string> OutputCondition
        {
            get
            {
                return m_outputCondition;
            }

            set
            {
                if (!Object.ReferenceEquals(m_outputCondition, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_outputCondition = value;
            }
        }

        /// <summary>
        /// A description for the StalePeriod Property.
        /// </summary>
        public PropertyState<uint> StalePeriod
        {
            get
            {
                return m_stalePeriod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_stalePeriod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_stalePeriod = value;
            }
        }

        /// <summary>
        /// A description for the Simulated Property.
        /// </summary>
        public PropertyState<bool> Simulated
        {
            get
            {
                return m_simulated;
            }

            set
            {
                if (!Object.ReferenceEquals(m_simulated, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_simulated = value;
            }
        }

        /// <summary>
        /// A description for the Locked Property.
        /// </summary>
        public PropertyState<bool> Locked
        {
            get
            {
                return m_locked;
            }

            set
            {
                if (!Object.ReferenceEquals(m_locked, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_locked = value;
            }
        }

        /// <summary>
        /// A description for the DataItemWriteManualMethodType Method.
        /// </summary>
        public MethodState WriteManual
        {
            get
            {
                return m_writeManualMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeManualMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeManualMethod = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_alias != null)
            {
                children.Add(m_alias);
            }

            if (m_severity != null)
            {
                children.Add(m_severity);
            }

            if (m_input1 != null)
            {
                children.Add(m_input1);
            }

            if (m_input2 != null)
            {
                children.Add(m_input2);
            }

            if (m_output != null)
            {
                children.Add(m_output);
            }

            if (m_outputCondition != null)
            {
                children.Add(m_outputCondition);
            }

            if (m_stalePeriod != null)
            {
                children.Add(m_stalePeriod);
            }

            if (m_simulated != null)
            {
                children.Add(m_simulated);
            }

            if (m_locked != null)
            {
                children.Add(m_locked);
            }

            if (m_writeManualMethod != null)
            {
                children.Add(m_writeManualMethod);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Alias:
                {
                    if (createOrReplace)
                    {
                        if (Alias == null)
                        {
                            if (replacement == null)
                            {
                                Alias = new PropertyState<string>(this);
                            }
                            else
                            {
                                Alias = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Alias;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Severity:
                {
                    if (createOrReplace)
                    {
                        if (Severity == null)
                        {
                            if (replacement == null)
                            {
                                Severity = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Severity = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Severity;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Input1:
                {
                    if (createOrReplace)
                    {
                        if (Input1 == null)
                        {
                            if (replacement == null)
                            {
                                Input1 = new PropertyState<string>(this);
                            }
                            else
                            {
                                Input1 = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Input1;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Input2:
                {
                    if (createOrReplace)
                    {
                        if (Input2 == null)
                        {
                            if (replacement == null)
                            {
                                Input2 = new PropertyState<string>(this);
                            }
                            else
                            {
                                Input2 = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Input2;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Output:
                {
                    if (createOrReplace)
                    {
                        if (Output == null)
                        {
                            if (replacement == null)
                            {
                                Output = new PropertyState<string>(this);
                            }
                            else
                            {
                                Output = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Output;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.OutputCondition:
                {
                    if (createOrReplace)
                    {
                        if (OutputCondition == null)
                        {
                            if (replacement == null)
                            {
                                OutputCondition = new PropertyState<string>(this);
                            }
                            else
                            {
                                OutputCondition = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = OutputCondition;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.StalePeriod:
                {
                    if (createOrReplace)
                    {
                        if (StalePeriod == null)
                        {
                            if (replacement == null)
                            {
                                StalePeriod = new PropertyState<uint>(this);
                            }
                            else
                            {
                                StalePeriod = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = StalePeriod;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Simulated:
                {
                    if (createOrReplace)
                    {
                        if (Simulated == null)
                        {
                            if (replacement == null)
                            {
                                Simulated = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Simulated = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Simulated;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Locked:
                {
                    if (createOrReplace)
                    {
                        if (Locked == null)
                        {
                            if (replacement == null)
                            {
                                Locked = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Locked = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Locked;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.WriteManual:
                {
                    if (createOrReplace)
                    {
                        if (WriteManual == null)
                        {
                            if (replacement == null)
                            {
                                WriteManual = new MethodState(this);
                            }
                            else
                            {
                                WriteManual = (MethodState)replacement;
                            }
                        }
                    }

                    instance = WriteManual;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_alias;
        private PropertyState<uint> m_severity;
        private PropertyState<string> m_input1;
        private PropertyState<string> m_input2;
        private PropertyState<string> m_output;
        private PropertyState<string> m_outputCondition;
        private PropertyState<uint> m_stalePeriod;
        private PropertyState<bool> m_simulated;
        private PropertyState<bool> m_locked;
        private MethodState m_writeManualMethod;
        #endregion
    }

    #region DataItemState<T> Class
    /// <summary>
    /// A typed version of the DataItemType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class DataItemState<T> : DataItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public DataItemState(NodeState parent) : base(parent)
        {
            Value = default(T);
        }

        /// <summary>
        /// Initializes the instance with the default values.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            base.Initialize(context);

            Value = default(T);
            DataType = TypeInfo.GetDataTypeId(typeof(T));
            ValueRank = TypeInfo.GetValueRank(typeof(T));
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The value of the variable.
        /// </summary>
        public new T Value
        {
            get
            {
                return CheckTypeBeforeCast<T>(base.Value, true);
            }

            set
            {
                base.Value = value;
            }
        }
        #endregion
    }
    #endregion
    #endif
    #endregion

    #region DiscreteItemState Class
    #if (!OPCUA_EXCLUDE_DiscreteItemState)
    /// <summary>
    /// Stores an instance of the DiscreteItemType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DiscreteItemState : DataItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DiscreteItemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.VariableTypes.DiscreteItemType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.BaseDataType, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////xVgiQACAAAAAQAYAAAA" +
           "RGlzY3JldGVJdGVtVHlwZUluc3RhbmNlAQFIAAEBSAAAGP////8BAQUAAAABAUUAAAEBQwABAUYAAAEB" +
           "FAABARcAAQEBPgABARcAAQEBGAABAUcAAAEBUAALAAAAFWCJCgIAAAABAAUAAABBbGlhcwEBSQAALgBE" +
           "SQAAAAAM/////wEB/////wAAAAAVYKkKAgAAAAEACAAAAFNldmVyaXR5AQG1AAAuAES1AAAABwEAAAAA" +
           "B/////8BAf////8AAAAAFWCJCgIAAAABAAYAAABJbnB1dDEBAbYAAC4ARLYAAAAADP////8BAf////8A" +
           "AAAAFWCJCgIAAAABAAYAAABJbnB1dDIBAbcAAC4ARLcAAAAADP////8BAf////8AAAAAFWCJCgIAAAAB" +
           "AAYAAABPdXRwdXQBAbgAAC4ARLgAAAAADP////8BAf////8AAAAAFWCJCgIAAAABAA8AAABPdXRwdXRD" +
           "b25kaXRpb24BAbkAAC4ARLkAAAAADP////8BAf////8AAAAAFWCJCgIAAAABAAsAAABTdGFsZVBlcmlv" +
           "ZAEBugAALgBEugAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEACQAAAFNpbXVsYXRlZAEBSgAALgBE" +
           "SgAAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEABgAAAExvY2tlZAEBuwAALgBEuwAAAAAB/////wEB" +
           "/////wAAAAAEYYIKBAAAAAEACwAAAFdyaXRlTWFudWFsAQEeAQAvAQEdAR4BAAABAf////8AAAAAFWCJ" +
           "CgIAAAABAAkAAABJbnZlcnNpb24BAbwAAC4ARLwAAAAAAf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Inversion Property.
        /// </summary>
        public PropertyState<bool> Inversion
        {
            get
            {
                return m_inversion;
            }

            set
            {
                if (!Object.ReferenceEquals(m_inversion, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_inversion = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_inversion != null)
            {
                children.Add(m_inversion);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Inversion:
                {
                    if (createOrReplace)
                    {
                        if (Inversion == null)
                        {
                            if (replacement == null)
                            {
                                Inversion = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Inversion = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Inversion;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<bool> m_inversion;
        #endregion
    }

    #region DiscreteItemState<T> Class
    /// <summary>
    /// A typed version of the DiscreteItemType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class DiscreteItemState<T> : DiscreteItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public DiscreteItemState(NodeState parent) : base(parent)
        {
            Value = default(T);
        }

        /// <summary>
        /// Initializes the instance with the default values.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            base.Initialize(context);

            Value = default(T);
            DataType = TypeInfo.GetDataTypeId(typeof(T));
            ValueRank = TypeInfo.GetValueRank(typeof(T));
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The value of the variable.
        /// </summary>
        public new T Value
        {
            get
            {
                return CheckTypeBeforeCast<T>(base.Value, true);
            }

            set
            {
                base.Value = value;
            }
        }
        #endregion
    }
    #endregion
    #endif
    #endregion

    #region AnalogItemState Class
    #if (!OPCUA_EXCLUDE_AnalogItemState)
    /// <summary>
    /// Stores an instance of the AnalogItemType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalogItemState : DataItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalogItemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.VariableTypes.AnalogItemType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.BaseDataType, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////xVgiQACAAAAAQAWAAAA" +
           "QW5hbG9nSXRlbVR5cGVJbnN0YW5jZQEBTAABAUwAABj/////AQEEAAAAAQFFAAABAUMAAQFGAAABARQA" +
           "AQEXAAEBAT4AAQEXAAEBARgAFgAAABVgiQoCAAAAAQAFAAAAQWxpYXMBAR8BAC4ARB8BAAAADP////8B" +
           "Af////8AAAAAFWCpCgIAAAABAAgAAABTZXZlcml0eQEBIAEALgBEIAEAAAcBAAAAAAf/////AQH/////" +
           "AAAAABVgiQoCAAAAAQAGAAAASW5wdXQxAQEhAQAuAEQhAQAAAAz/////AQH/////AAAAABVgiQoCAAAA" +
           "AQAGAAAASW5wdXQyAQEiAQAuAEQiAQAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAGAAAAT3V0cHV0" +
           "AQEjAQAuAEQjAQAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAPAAAAT3V0cHV0Q29uZGl0aW9uAQEk" +
           "AQAuAEQkAQAAAAz/////AQH/////AAAAABVgiQoCAAAAAQALAAAAU3RhbGVQZXJpb2QBASUBAC4ARCUB" +
           "AAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAkAAABTaW11bGF0ZWQBASYBAC4ARCYBAAAAAf////8B" +
           "Af////8AAAAAFWCJCgIAAAABAAYAAABMb2NrZWQBAScBAC4ARCcBAAAAAf////8BAf////8AAAAABGGC" +
           "CgQAAAABAAsAAABXcml0ZU1hbnVhbAEBKAEALwEBHQEoAQAAAQH/////AAAAABVgiQoCAAAAAQANAAAA" +
           "RGlzcGxheUZvcm1hdAEBvQAALgBEvQAAAAAM/////wEB/////wAAAAAVYIkKAgAAAAEACgAAAENvbnZl" +
           "cnNpb24BAb4AAC4ARL4AAAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAgAAABDbGFtcGluZwEBvwAA" +
           "LgBEvwAAAAAB/////wEB/////wAAAAAVYKkKAgAAAAEABAAAAEV1TG8BAcAAAC4ARMAAAAALAAAAAAAA" +
           "WcAAC/////8BAf////8AAAAAFWCpCgIAAAABAAQAAABFdUhpAQHBAAAuAETBAAAACwAAAAAAAFlAAAv/" +
           "////AQH/////AAAAABVgqQoCAAAAAQAEAAAASXJMbwEBwgAALgBEwgAAAAsAAAAAAADwPwAL/////wEB" +
           "/////wAAAAAVYKkKAgAAAAEABAAAAElySGkBAcMAAC4ARMMAAAALAAAAAOD/70AAC/////8BAf////8A" +
           "AAAAFWCJCgIAAAABAAcAAABMaW1pdExvAQHEAAAuAETEAAAAAAv/////AQH/////AAAAABVgiQoCAAAA" +
           "AQAHAAAATGltaXRIaQEBxQAALgBExQAAAAAL/////wEB/////wAAAAAVYIkKAgAAAAEACQAAAExpbWl0" +
           "TG9MbwEBxgAALgBExgAAAAAL/////wEB/////wAAAAAVYIkKAgAAAAEACQAAAExpbWl0SGlIaQEBxwAA" +
           "LgBExwAAAAAL/////wEB/////wAAAAAVYIkKAgAAAAEAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAcgAAC4A" +
           "RMgAAAAADP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the DisplayFormat Property.
        /// </summary>
        public PropertyState<string> DisplayFormat
        {
            get
            {
                return m_displayFormat;
            }

            set
            {
                if (!Object.ReferenceEquals(m_displayFormat, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_displayFormat = value;
            }
        }

        /// <summary>
        /// A description for the Conversion Property.
        /// </summary>
        public PropertyState<uint> Conversion
        {
            get
            {
                return m_conversion;
            }

            set
            {
                if (!Object.ReferenceEquals(m_conversion, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_conversion = value;
            }
        }

        /// <summary>
        /// A description for the Clamping Property.
        /// </summary>
        public PropertyState<bool> Clamping
        {
            get
            {
                return m_clamping;
            }

            set
            {
                if (!Object.ReferenceEquals(m_clamping, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_clamping = value;
            }
        }

        /// <summary>
        /// A description for the EuLo Property.
        /// </summary>
        public PropertyState<double> EuLo
        {
            get
            {
                return m_euLo;
            }

            set
            {
                if (!Object.ReferenceEquals(m_euLo, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_euLo = value;
            }
        }

        /// <summary>
        /// A description for the EuHi Property.
        /// </summary>
        public PropertyState<double> EuHi
        {
            get
            {
                return m_euHi;
            }

            set
            {
                if (!Object.ReferenceEquals(m_euHi, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_euHi = value;
            }
        }

        /// <summary>
        /// A description for the IrLo Property.
        /// </summary>
        public PropertyState<double> IrLo
        {
            get
            {
                return m_irLo;
            }

            set
            {
                if (!Object.ReferenceEquals(m_irLo, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_irLo = value;
            }
        }

        /// <summary>
        /// A description for the IrHi Property.
        /// </summary>
        public PropertyState<double> IrHi
        {
            get
            {
                return m_irHi;
            }

            set
            {
                if (!Object.ReferenceEquals(m_irHi, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_irHi = value;
            }
        }

        /// <summary>
        /// A description for the LimitLo Property.
        /// </summary>
        public PropertyState<double> LimitLo
        {
            get
            {
                return m_limitLo;
            }

            set
            {
                if (!Object.ReferenceEquals(m_limitLo, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_limitLo = value;
            }
        }

        /// <summary>
        /// A description for the LimitHi Property.
        /// </summary>
        public PropertyState<double> LimitHi
        {
            get
            {
                return m_limitHi;
            }

            set
            {
                if (!Object.ReferenceEquals(m_limitHi, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_limitHi = value;
            }
        }

        /// <summary>
        /// A description for the LimitLoLo Property.
        /// </summary>
        public PropertyState<double> LimitLoLo
        {
            get
            {
                return m_limitLoLo;
            }

            set
            {
                if (!Object.ReferenceEquals(m_limitLoLo, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_limitLoLo = value;
            }
        }

        /// <summary>
        /// A description for the LimitHiHi Property.
        /// </summary>
        public PropertyState<double> LimitHiHi
        {
            get
            {
                return m_limitHiHi;
            }

            set
            {
                if (!Object.ReferenceEquals(m_limitHiHi, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_limitHiHi = value;
            }
        }

        /// <summary>
        /// A description for the EngineeringUnits Property.
        /// </summary>
        public PropertyState<string> EngineeringUnits
        {
            get
            {
                return m_engineeringUnits;
            }

            set
            {
                if (!Object.ReferenceEquals(m_engineeringUnits, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_engineeringUnits = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_displayFormat != null)
            {
                children.Add(m_displayFormat);
            }

            if (m_conversion != null)
            {
                children.Add(m_conversion);
            }

            if (m_clamping != null)
            {
                children.Add(m_clamping);
            }

            if (m_euLo != null)
            {
                children.Add(m_euLo);
            }

            if (m_euHi != null)
            {
                children.Add(m_euHi);
            }

            if (m_irLo != null)
            {
                children.Add(m_irLo);
            }

            if (m_irHi != null)
            {
                children.Add(m_irHi);
            }

            if (m_limitLo != null)
            {
                children.Add(m_limitLo);
            }

            if (m_limitHi != null)
            {
                children.Add(m_limitHi);
            }

            if (m_limitLoLo != null)
            {
                children.Add(m_limitLoLo);
            }

            if (m_limitHiHi != null)
            {
                children.Add(m_limitHiHi);
            }

            if (m_engineeringUnits != null)
            {
                children.Add(m_engineeringUnits);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.DisplayFormat:
                {
                    if (createOrReplace)
                    {
                        if (DisplayFormat == null)
                        {
                            if (replacement == null)
                            {
                                DisplayFormat = new PropertyState<string>(this);
                            }
                            else
                            {
                                DisplayFormat = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DisplayFormat;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Conversion:
                {
                    if (createOrReplace)
                    {
                        if (Conversion == null)
                        {
                            if (replacement == null)
                            {
                                Conversion = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Conversion = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Conversion;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Clamping:
                {
                    if (createOrReplace)
                    {
                        if (Clamping == null)
                        {
                            if (replacement == null)
                            {
                                Clamping = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Clamping = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Clamping;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.EuLo:
                {
                    if (createOrReplace)
                    {
                        if (EuLo == null)
                        {
                            if (replacement == null)
                            {
                                EuLo = new PropertyState<double>(this);
                            }
                            else
                            {
                                EuLo = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = EuLo;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.EuHi:
                {
                    if (createOrReplace)
                    {
                        if (EuHi == null)
                        {
                            if (replacement == null)
                            {
                                EuHi = new PropertyState<double>(this);
                            }
                            else
                            {
                                EuHi = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = EuHi;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.IrLo:
                {
                    if (createOrReplace)
                    {
                        if (IrLo == null)
                        {
                            if (replacement == null)
                            {
                                IrLo = new PropertyState<double>(this);
                            }
                            else
                            {
                                IrLo = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = IrLo;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.IrHi:
                {
                    if (createOrReplace)
                    {
                        if (IrHi == null)
                        {
                            if (replacement == null)
                            {
                                IrHi = new PropertyState<double>(this);
                            }
                            else
                            {
                                IrHi = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = IrHi;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.LimitLo:
                {
                    if (createOrReplace)
                    {
                        if (LimitLo == null)
                        {
                            if (replacement == null)
                            {
                                LimitLo = new PropertyState<double>(this);
                            }
                            else
                            {
                                LimitLo = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = LimitLo;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.LimitHi:
                {
                    if (createOrReplace)
                    {
                        if (LimitHi == null)
                        {
                            if (replacement == null)
                            {
                                LimitHi = new PropertyState<double>(this);
                            }
                            else
                            {
                                LimitHi = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = LimitHi;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.LimitLoLo:
                {
                    if (createOrReplace)
                    {
                        if (LimitLoLo == null)
                        {
                            if (replacement == null)
                            {
                                LimitLoLo = new PropertyState<double>(this);
                            }
                            else
                            {
                                LimitLoLo = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = LimitLoLo;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.LimitHiHi:
                {
                    if (createOrReplace)
                    {
                        if (LimitHiHi == null)
                        {
                            if (replacement == null)
                            {
                                LimitHiHi = new PropertyState<double>(this);
                            }
                            else
                            {
                                LimitHiHi = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = LimitHiHi;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.EngineeringUnits:
                {
                    if (createOrReplace)
                    {
                        if (EngineeringUnits == null)
                        {
                            if (replacement == null)
                            {
                                EngineeringUnits = new PropertyState<string>(this);
                            }
                            else
                            {
                                EngineeringUnits = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = EngineeringUnits;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_displayFormat;
        private PropertyState<uint> m_conversion;
        private PropertyState<bool> m_clamping;
        private PropertyState<double> m_euLo;
        private PropertyState<double> m_euHi;
        private PropertyState<double> m_irLo;
        private PropertyState<double> m_irHi;
        private PropertyState<double> m_limitLo;
        private PropertyState<double> m_limitHi;
        private PropertyState<double> m_limitLoLo;
        private PropertyState<double> m_limitHiHi;
        private PropertyState<string> m_engineeringUnits;
        #endregion
    }

    #region AnalogItemState<T> Class
    /// <summary>
    /// A typed version of the AnalogItemType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class AnalogItemState<T> : AnalogItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public AnalogItemState(NodeState parent) : base(parent)
        {
            Value = default(T);
        }

        /// <summary>
        /// Initializes the instance with the default values.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            base.Initialize(context);

            Value = default(T);
            DataType = TypeInfo.GetDataTypeId(typeof(T));
            ValueRank = TypeInfo.GetValueRank(typeof(T));
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The value of the variable.
        /// </summary>
        public new T Value
        {
            get
            {
                return CheckTypeBeforeCast<T>(base.Value, true);
            }

            set
            {
                base.Value = value;
            }
        }
        #endregion
    }
    #endregion
    #endif
    #endregion

    #region TsFormatState Class
    #if (!OPCUA_EXCLUDE_TsFormatState)
    /// <summary>
    /// Stores an instance of the TsFormatType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TsFormatState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TsFormatState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.TsFormatType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAUAAAA" +
           "VHNGb3JtYXRUeXBlSW5zdGFuY2UBAVAAAQFQAAEAAAABARcAAQEBGwAEAAAAFWCJCgIAAAABAAkAAABP" +
           "cGVuTGFiZWwBAVEAAC4ARFEAAAAADP////8BAf////8AAAAAFWCJCgIAAAABAAoAAABDbG9zZUxhYmVs" +
           "AQFSAAAuAERSAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAJAAAAT3BlbkNvbG9yAQFTAAAuAERT" +
           "AAAAAAf/////AQH/////AAAAABVgiQoCAAAAAQAKAAAAQ2xvc2VDb2xvcgEBVAAALgBEVAAAAAAH////" +
           "/wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the OpenLabel Property.
        /// </summary>
        public PropertyState<string> OpenLabel
        {
            get
            {
                return m_openLabel;
            }

            set
            {
                if (!Object.ReferenceEquals(m_openLabel, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_openLabel = value;
            }
        }

        /// <summary>
        /// A description for the CloseLabel Property.
        /// </summary>
        public PropertyState<string> CloseLabel
        {
            get
            {
                return m_closeLabel;
            }

            set
            {
                if (!Object.ReferenceEquals(m_closeLabel, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_closeLabel = value;
            }
        }

        /// <summary>
        /// A description for the OpenColor Property.
        /// </summary>
        public PropertyState<uint> OpenColor
        {
            get
            {
                return m_openColor;
            }

            set
            {
                if (!Object.ReferenceEquals(m_openColor, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_openColor = value;
            }
        }

        /// <summary>
        /// A description for the CloseColor Property.
        /// </summary>
        public PropertyState<uint> CloseColor
        {
            get
            {
                return m_closeColor;
            }

            set
            {
                if (!Object.ReferenceEquals(m_closeColor, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_closeColor = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_openLabel != null)
            {
                children.Add(m_openLabel);
            }

            if (m_closeLabel != null)
            {
                children.Add(m_closeLabel);
            }

            if (m_openColor != null)
            {
                children.Add(m_openColor);
            }

            if (m_closeColor != null)
            {
                children.Add(m_closeColor);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.OpenLabel:
                {
                    if (createOrReplace)
                    {
                        if (OpenLabel == null)
                        {
                            if (replacement == null)
                            {
                                OpenLabel = new PropertyState<string>(this);
                            }
                            else
                            {
                                OpenLabel = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = OpenLabel;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.CloseLabel:
                {
                    if (createOrReplace)
                    {
                        if (CloseLabel == null)
                        {
                            if (replacement == null)
                            {
                                CloseLabel = new PropertyState<string>(this);
                            }
                            else
                            {
                                CloseLabel = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = CloseLabel;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.OpenColor:
                {
                    if (createOrReplace)
                    {
                        if (OpenColor == null)
                        {
                            if (replacement == null)
                            {
                                OpenColor = new PropertyState<uint>(this);
                            }
                            else
                            {
                                OpenColor = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = OpenColor;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.CloseColor:
                {
                    if (createOrReplace)
                    {
                        if (CloseColor == null)
                        {
                            if (replacement == null)
                            {
                                CloseColor = new PropertyState<uint>(this);
                            }
                            else
                            {
                                CloseColor = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = CloseColor;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_openLabel;
        private PropertyState<string> m_closeLabel;
        private PropertyState<uint> m_openColor;
        private PropertyState<uint> m_closeColor;
        #endregion
    }
    #endif
    #endregion

    #region SimulationSignalState Class
    #if (!OPCUA_EXCLUDE_SimulationSignalState)
    /// <summary>
    /// Stores an instance of the SimulationSignalType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SimulationSignalState : BaseVariableState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SimulationSignalState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.VariableTypes.SimulationSignalType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.BaseDataType, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////xVggQACAAAAAQAcAAAA" +
           "U2ltdWxhdGlvblNpZ25hbFR5cGVJbnN0YW5jZQEBQwABAUMAABgBAQEAAAABARcAAQEBHAAEAAAAFWCJ" +
           "CgIAAAABAAQAAABUeXBlAQHlAAAuAETlAAAAAAf/////AQH/////AAAAABVgqQoCAAAAAQAGAAAAUGVy" +
           "aW9kAQHmAAAuAETmAAAACwAAAAAATO1AAAf/////AQH/////AAAAABVgiQoCAAAAAQAFAAAAUGhhc2UB" +
           "AecAAC4AROcAAAAAB/////8BAf////8AAAAAFWCpCgIAAAABAA4AAABVcGRhdGVJbnRlcnZhbAEB6AAA" +
           "LgBE6AAAAAsAAAAAAECPQAAH/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Type Property.
        /// </summary>
        public PropertyState<uint> Type
        {
            get
            {
                return m_type;
            }

            set
            {
                if (!Object.ReferenceEquals(m_type, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_type = value;
            }
        }

        /// <summary>
        /// A description for the Period Property.
        /// </summary>
        public PropertyState<uint> Period
        {
            get
            {
                return m_period;
            }

            set
            {
                if (!Object.ReferenceEquals(m_period, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_period = value;
            }
        }

        /// <summary>
        /// A description for the Phase Property.
        /// </summary>
        public PropertyState<uint> Phase
        {
            get
            {
                return m_phase;
            }

            set
            {
                if (!Object.ReferenceEquals(m_phase, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_phase = value;
            }
        }

        /// <summary>
        /// A description for the UpdateInterval Property.
        /// </summary>
        public PropertyState<uint> UpdateInterval
        {
            get
            {
                return m_updateInterval;
            }

            set
            {
                if (!Object.ReferenceEquals(m_updateInterval, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_updateInterval = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_type != null)
            {
                children.Add(m_type);
            }

            if (m_period != null)
            {
                children.Add(m_period);
            }

            if (m_phase != null)
            {
                children.Add(m_phase);
            }

            if (m_updateInterval != null)
            {
                children.Add(m_updateInterval);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Type:
                {
                    if (createOrReplace)
                    {
                        if (Type == null)
                        {
                            if (replacement == null)
                            {
                                Type = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Type = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Type;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Period:
                {
                    if (createOrReplace)
                    {
                        if (Period == null)
                        {
                            if (replacement == null)
                            {
                                Period = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Period = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Period;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Phase:
                {
                    if (createOrReplace)
                    {
                        if (Phase == null)
                        {
                            if (replacement == null)
                            {
                                Phase = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Phase = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Phase;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.UpdateInterval:
                {
                    if (createOrReplace)
                    {
                        if (UpdateInterval == null)
                        {
                            if (replacement == null)
                            {
                                UpdateInterval = new PropertyState<uint>(this);
                            }
                            else
                            {
                                UpdateInterval = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = UpdateInterval;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_type;
        private PropertyState<uint> m_period;
        private PropertyState<uint> m_phase;
        private PropertyState<uint> m_updateInterval;
        #endregion
    }

    #region SimulationSignalState<T> Class
    /// <summary>
    /// A typed version of the SimulationSignalType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class SimulationSignalState<T> : SimulationSignalState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public SimulationSignalState(NodeState parent) : base(parent)
        {
            Value = default(T);
        }

        /// <summary>
        /// Initializes the instance with the default values.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            base.Initialize(context);

            Value = default(T);
            DataType = TypeInfo.GetDataTypeId(typeof(T));
            ValueRank = TypeInfo.GetValueRank(typeof(T));
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The value of the variable.
        /// </summary>
        public new T Value
        {
            get
            {
                return CheckTypeBeforeCast<T>(base.Value, true);
            }

            set
            {
                base.Value = value;
            }
        }
        #endregion
    }
    #endregion
    #endif
    #endregion

    #region UserState Class
    #if (!OPCUA_EXCLUDE_UserState)
    /// <summary>
    /// Stores an instance of the UserType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class UserState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public UserState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.UserType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAQAAAA" +
           "VXNlclR5cGVJbnN0YW5jZQEBEAABARAAAQAAAAEBFwABAQEdAAEAAAAVYKkKAgAAAAEADAAAAEFjY2Vz" +
           "c1JpZ2h0cwEBEQAALgBEEQAAAAcAAAAAAAf/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the AccessRights Property.
        /// </summary>
        public PropertyState<uint> AccessRights
        {
            get
            {
                return m_accessRights;
            }

            set
            {
                if (!Object.ReferenceEquals(m_accessRights, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_accessRights = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_accessRights != null)
            {
                children.Add(m_accessRights);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.AccessRights:
                {
                    if (createOrReplace)
                    {
                        if (AccessRights == null)
                        {
                            if (replacement == null)
                            {
                                AccessRights = new PropertyState<uint>(this);
                            }
                            else
                            {
                                AccessRights = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = AccessRights;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_accessRights;
        #endregion
    }
    #endif
    #endregion

    #region HistoricalDatabaseState Class
    #if (!OPCUA_EXCLUDE_HistoricalDatabaseState)
    /// <summary>
    /// Stores an instance of the HistoricalDatabaseType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class HistoricalDatabaseState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public HistoricalDatabaseState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.HistoricalDatabaseType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAeAAAA" +
           "SGlzdG9yaWNhbERhdGFiYXNlVHlwZUluc3RhbmNlAQEUAAEBFAABAAAAAQEXAAEBAQwABQAAABVgqQoC" +
           "AAAAAQAFAAAARGVwdGgBASQAAC4ARCQAAAAHAQAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAEgAA" +
           "AFdyaXRlVmFsdWVEdXJhdGlvbgEBMQAALwA/MQAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAEAAA" +
           "AFBlbmRpbmdUYXNrQ291bnQBATIAAC8APzIAAAAAB/////8BAf////8AAAAAFWCJCgIAAAABABQAAABF" +
           "dmVudENsZWFudXBEdXJhdGlvbgEBMwAALwA/MwAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAFAAA" +
           "AFZhbHVlQ2xlYW51cER1cmF0aW9uAQE0AAAvAD80AAAAAAf/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Depth Property.
        /// </summary>
        public PropertyState<uint> Depth
        {
            get
            {
                return m_depth;
            }

            set
            {
                if (!Object.ReferenceEquals(m_depth, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_depth = value;
            }
        }

        /// <summary>
        /// A description for the WriteValueDuration Variable.
        /// </summary>
        public BaseDataVariableState<uint> WriteValueDuration
        {
            get
            {
                return m_writeValueDuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeValueDuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeValueDuration = value;
            }
        }

        /// <summary>
        /// A description for the PendingTaskCount Variable.
        /// </summary>
        public BaseDataVariableState<uint> PendingTaskCount
        {
            get
            {
                return m_pendingTaskCount;
            }

            set
            {
                if (!Object.ReferenceEquals(m_pendingTaskCount, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_pendingTaskCount = value;
            }
        }

        /// <summary>
        /// A description for the EventCleanupDuration Variable.
        /// </summary>
        public BaseDataVariableState<uint> EventCleanupDuration
        {
            get
            {
                return m_eventCleanupDuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_eventCleanupDuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_eventCleanupDuration = value;
            }
        }

        /// <summary>
        /// A description for the ValueCleanupDuration Variable.
        /// </summary>
        public BaseDataVariableState<uint> ValueCleanupDuration
        {
            get
            {
                return m_valueCleanupDuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_valueCleanupDuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_valueCleanupDuration = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_depth != null)
            {
                children.Add(m_depth);
            }

            if (m_writeValueDuration != null)
            {
                children.Add(m_writeValueDuration);
            }

            if (m_pendingTaskCount != null)
            {
                children.Add(m_pendingTaskCount);
            }

            if (m_eventCleanupDuration != null)
            {
                children.Add(m_eventCleanupDuration);
            }

            if (m_valueCleanupDuration != null)
            {
                children.Add(m_valueCleanupDuration);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Depth:
                {
                    if (createOrReplace)
                    {
                        if (Depth == null)
                        {
                            if (replacement == null)
                            {
                                Depth = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Depth = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Depth;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.WriteValueDuration:
                {
                    if (createOrReplace)
                    {
                        if (WriteValueDuration == null)
                        {
                            if (replacement == null)
                            {
                                WriteValueDuration = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                WriteValueDuration = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = WriteValueDuration;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.PendingTaskCount:
                {
                    if (createOrReplace)
                    {
                        if (PendingTaskCount == null)
                        {
                            if (replacement == null)
                            {
                                PendingTaskCount = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                PendingTaskCount = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = PendingTaskCount;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.EventCleanupDuration:
                {
                    if (createOrReplace)
                    {
                        if (EventCleanupDuration == null)
                        {
                            if (replacement == null)
                            {
                                EventCleanupDuration = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                EventCleanupDuration = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = EventCleanupDuration;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ValueCleanupDuration:
                {
                    if (createOrReplace)
                    {
                        if (ValueCleanupDuration == null)
                        {
                            if (replacement == null)
                            {
                                ValueCleanupDuration = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                ValueCleanupDuration = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = ValueCleanupDuration;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_depth;
        private BaseDataVariableState<uint> m_writeValueDuration;
        private BaseDataVariableState<uint> m_pendingTaskCount;
        private BaseDataVariableState<uint> m_eventCleanupDuration;
        private BaseDataVariableState<uint> m_valueCleanupDuration;
        #endregion
    }
    #endif
    #endregion

    #region DeviceState Class
    #if (!OPCUA_EXCLUDE_DeviceState)
    /// <summary>
    /// Stores an instance of the DeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DeviceState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.DeviceType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQASAAAA" +
           "RGV2aWNlVHlwZUluc3RhbmNlAQEfAAEBHwD/////CQAAABVgiQoCAAAAAQAIAAAARGlzYWJsZWQBAckA" +
           "AC4ARMkAAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAYAAABPbmxpbmUBAcoAAC8AP8oAAAAAAf//" +
           "//8BAf////8AAAAAFWCJCgIAAAABAAcAAABFbmFibGVkAQHLAAAvAD/LAAAAAAH/////AQH/////AAAA" +
           "AARhggoEAAAAAQALAAAASW50ZXJyb2dhdGUBAesAAC8BAesA6wAAAAEB/////wAAAAAEYYIKBAAAAAEA" +
           "EgAAAEludGVycm9nYXRlQ2hhbm5lbAEBFgEALwEBFgEWAQAAAQH/////AAAAAARhggoEAAAAAQAJAAAA" +
           "U3luY0Nsb2NrAQHsAAAvAQHsAOwAAAABAf////8AAAAABGGCCgQAAAABAAYAAABTZWxlY3QBAQABAC8B" +
           "AQABAAEAAAEB/////wAAAAAEYYIKBAAAAAEABQAAAFdyaXRlAQEBAQAvAQEBAQEBAAABAf////8AAAAA" +
           "BGGCCgQAAAABAAoAAABXcml0ZVBhcmFtAQECAQAvAQECAQIBAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Disabled Property.
        /// </summary>
        public PropertyState<bool> Disabled
        {
            get
            {
                return m_disabled;
            }

            set
            {
                if (!Object.ReferenceEquals(m_disabled, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_disabled = value;
            }
        }

        /// <summary>
        /// A description for the Online Variable.
        /// </summary>
        public BaseDataVariableState<bool> Online
        {
            get
            {
                return m_online;
            }

            set
            {
                if (!Object.ReferenceEquals(m_online, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_online = value;
            }
        }

        /// <summary>
        /// A description for the Enabled Variable.
        /// </summary>
        public BaseDataVariableState<bool> Enabled
        {
            get
            {
                return m_enabled;
            }

            set
            {
                if (!Object.ReferenceEquals(m_enabled, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_enabled = value;
            }
        }

        /// <summary>
        /// A description for the DeviceInterrogateMethodType Method.
        /// </summary>
        public MethodState Interrogate
        {
            get
            {
                return m_interrogateMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogateMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogateMethod = value;
            }
        }

        /// <summary>
        /// A description for the DeviceInterrogateChannelMethodType Method.
        /// </summary>
        public MethodState InterrogateChannel
        {
            get
            {
                return m_interrogateChannelMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogateChannelMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogateChannelMethod = value;
            }
        }

        /// <summary>
        /// A description for the DeviceSyncClockMethodType Method.
        /// </summary>
        public MethodState SyncClock
        {
            get
            {
                return m_syncClockMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_syncClockMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_syncClockMethod = value;
            }
        }

        /// <summary>
        /// A description for the DeviceSelectMethodType Method.
        /// </summary>
        public MethodState Select
        {
            get
            {
                return m_selectMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_selectMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_selectMethod = value;
            }
        }

        /// <summary>
        /// A description for the DeviceWriteMethodType Method.
        /// </summary>
        public MethodState Write
        {
            get
            {
                return m_writeMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeMethod = value;
            }
        }

        /// <summary>
        /// A description for the DeviceWriteParamMethodType Method.
        /// </summary>
        public MethodState WriteParam
        {
            get
            {
                return m_writeParamMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeParamMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeParamMethod = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_disabled != null)
            {
                children.Add(m_disabled);
            }

            if (m_online != null)
            {
                children.Add(m_online);
            }

            if (m_enabled != null)
            {
                children.Add(m_enabled);
            }

            if (m_interrogateMethod != null)
            {
                children.Add(m_interrogateMethod);
            }

            if (m_interrogateChannelMethod != null)
            {
                children.Add(m_interrogateChannelMethod);
            }

            if (m_syncClockMethod != null)
            {
                children.Add(m_syncClockMethod);
            }

            if (m_selectMethod != null)
            {
                children.Add(m_selectMethod);
            }

            if (m_writeMethod != null)
            {
                children.Add(m_writeMethod);
            }

            if (m_writeParamMethod != null)
            {
                children.Add(m_writeParamMethod);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Disabled:
                {
                    if (createOrReplace)
                    {
                        if (Disabled == null)
                        {
                            if (replacement == null)
                            {
                                Disabled = new PropertyState<bool>(this);
                            }
                            else
                            {
                                Disabled = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = Disabled;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Online:
                {
                    if (createOrReplace)
                    {
                        if (Online == null)
                        {
                            if (replacement == null)
                            {
                                Online = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Online = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Online;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Enabled:
                {
                    if (createOrReplace)
                    {
                        if (Enabled == null)
                        {
                            if (replacement == null)
                            {
                                Enabled = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Enabled = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Enabled;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Interrogate:
                {
                    if (createOrReplace)
                    {
                        if (Interrogate == null)
                        {
                            if (replacement == null)
                            {
                                Interrogate = new MethodState(this);
                            }
                            else
                            {
                                Interrogate = (MethodState)replacement;
                            }
                        }
                    }

                    instance = Interrogate;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogateChannel:
                {
                    if (createOrReplace)
                    {
                        if (InterrogateChannel == null)
                        {
                            if (replacement == null)
                            {
                                InterrogateChannel = new MethodState(this);
                            }
                            else
                            {
                                InterrogateChannel = (MethodState)replacement;
                            }
                        }
                    }

                    instance = InterrogateChannel;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.SyncClock:
                {
                    if (createOrReplace)
                    {
                        if (SyncClock == null)
                        {
                            if (replacement == null)
                            {
                                SyncClock = new MethodState(this);
                            }
                            else
                            {
                                SyncClock = (MethodState)replacement;
                            }
                        }
                    }

                    instance = SyncClock;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Select:
                {
                    if (createOrReplace)
                    {
                        if (Select == null)
                        {
                            if (replacement == null)
                            {
                                Select = new MethodState(this);
                            }
                            else
                            {
                                Select = (MethodState)replacement;
                            }
                        }
                    }

                    instance = Select;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Write:
                {
                    if (createOrReplace)
                    {
                        if (Write == null)
                        {
                            if (replacement == null)
                            {
                                Write = new MethodState(this);
                            }
                            else
                            {
                                Write = (MethodState)replacement;
                            }
                        }
                    }

                    instance = Write;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.WriteParam:
                {
                    if (createOrReplace)
                    {
                        if (WriteParam == null)
                        {
                            if (replacement == null)
                            {
                                WriteParam = new MethodState(this);
                            }
                            else
                            {
                                WriteParam = (MethodState)replacement;
                            }
                        }
                    }

                    instance = WriteParam;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<bool> m_disabled;
        private BaseDataVariableState<bool> m_online;
        private BaseDataVariableState<bool> m_enabled;
        private MethodState m_interrogateMethod;
        private MethodState m_interrogateChannelMethod;
        private MethodState m_syncClockMethod;
        private MethodState m_selectMethod;
        private MethodState m_writeMethod;
        private MethodState m_writeParamMethod;
        #endregion
    }
    #endif
    #endregion

    #region LinkState Class
    #if (!OPCUA_EXCLUDE_LinkState)
    /// <summary>
    /// Stores an instance of the LinkType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class LinkState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public LinkState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.LinkType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAQAAAA" +
           "TGlua1R5cGVJbnN0YW5jZQEBKAABASgAAQAAAAEBFwABAQEeABAAAAAVYIkKAgAAAAEACAAAAERpc2Fi" +
           "bGVkAQHMAAAuAETMAAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAGAAAAT25saW5lAQHNAAAvAD/N" +
           "AAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAHAAAARW5hYmxlZAEBzgAALwA/zgAAAAAB/////wEB" +
           "/////wAAAAAEYYIKBAAAAAEACwAAAEludGVycm9nYXRlAQHtAAAvAQHrAO0AAAABAf////8AAAAABGGC" +
           "CgQAAAABABIAAABJbnRlcnJvZ2F0ZUNoYW5uZWwBARcBAC8BARYBFwEAAAEB/////wAAAAAEYYIKBAAA" +
           "AAEACQAAAFN5bmNDbG9jawEB7gAALwEB7ADuAAAAAQH/////AAAAAARhggoEAAAAAQAGAAAAU2VsZWN0" +
           "AQEDAQAvAQEAAQMBAAABAf////8AAAAABGGCCgQAAAABAAUAAABXcml0ZQEBBAEALwEBAQEEAQAAAQH/" +
           "////AAAAAARhggoEAAAAAQAKAAAAV3JpdGVQYXJhbQEBBQEALwEBAgEFAQAAAQH/////AAAAABVgiQoC" +
           "AAAAAQAJAAAAVHJhbnNwb3J0AQEqAAAuAEQqAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAQAMAAAA" +
           "Q29ubmVjdENvdW50AQErAAAvAD8rAAAAAAf/////AQH/////AAAAABVgiQoCAAAAAQARAAAAQWN0aXZl" +
           "Q29ubmVjdGlvbnMBASwAAC8APywAAAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAsAAABNZXNzYWdl" +
           "c091dAEBLQAALwA/LQAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEACgAAAE1lc3NhZ2VzSW4BAS4A" +
           "AC8APy4AAAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAgAAABCeXRlc091dAEBLwAALwA/LwAAAAAH" +
           "/////wEB/////wAAAAAVYIkKAgAAAAEABwAAAEJ5dGVzSW4BATAAAC8APzAAAAAAB/////8BAf////8A" +
           "AAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Transport Property.
        /// </summary>
        public PropertyState<string> Transport
        {
            get
            {
                return m_transport;
            }

            set
            {
                if (!Object.ReferenceEquals(m_transport, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_transport = value;
            }
        }

        /// <summary>
        /// A description for the ConnectCount Variable.
        /// </summary>
        public BaseDataVariableState<uint> ConnectCount
        {
            get
            {
                return m_connectCount;
            }

            set
            {
                if (!Object.ReferenceEquals(m_connectCount, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_connectCount = value;
            }
        }

        /// <summary>
        /// A description for the ActiveConnections Variable.
        /// </summary>
        public BaseDataVariableState<uint> ActiveConnections
        {
            get
            {
                return m_activeConnections;
            }

            set
            {
                if (!Object.ReferenceEquals(m_activeConnections, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_activeConnections = value;
            }
        }

        /// <summary>
        /// A description for the MessagesOut Variable.
        /// </summary>
        public BaseDataVariableState<uint> MessagesOut
        {
            get
            {
                return m_messagesOut;
            }

            set
            {
                if (!Object.ReferenceEquals(m_messagesOut, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_messagesOut = value;
            }
        }

        /// <summary>
        /// A description for the MessagesIn Variable.
        /// </summary>
        public BaseDataVariableState<uint> MessagesIn
        {
            get
            {
                return m_messagesIn;
            }

            set
            {
                if (!Object.ReferenceEquals(m_messagesIn, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_messagesIn = value;
            }
        }

        /// <summary>
        /// A description for the BytesOut Variable.
        /// </summary>
        public BaseDataVariableState<uint> BytesOut
        {
            get
            {
                return m_bytesOut;
            }

            set
            {
                if (!Object.ReferenceEquals(m_bytesOut, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_bytesOut = value;
            }
        }

        /// <summary>
        /// A description for the BytesIn Variable.
        /// </summary>
        public BaseDataVariableState<uint> BytesIn
        {
            get
            {
                return m_bytesIn;
            }

            set
            {
                if (!Object.ReferenceEquals(m_bytesIn, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_bytesIn = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_transport != null)
            {
                children.Add(m_transport);
            }

            if (m_connectCount != null)
            {
                children.Add(m_connectCount);
            }

            if (m_activeConnections != null)
            {
                children.Add(m_activeConnections);
            }

            if (m_messagesOut != null)
            {
                children.Add(m_messagesOut);
            }

            if (m_messagesIn != null)
            {
                children.Add(m_messagesIn);
            }

            if (m_bytesOut != null)
            {
                children.Add(m_bytesOut);
            }

            if (m_bytesIn != null)
            {
                children.Add(m_bytesIn);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Transport:
                {
                    if (createOrReplace)
                    {
                        if (Transport == null)
                        {
                            if (replacement == null)
                            {
                                Transport = new PropertyState<string>(this);
                            }
                            else
                            {
                                Transport = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Transport;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ConnectCount:
                {
                    if (createOrReplace)
                    {
                        if (ConnectCount == null)
                        {
                            if (replacement == null)
                            {
                                ConnectCount = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                ConnectCount = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = ConnectCount;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ActiveConnections:
                {
                    if (createOrReplace)
                    {
                        if (ActiveConnections == null)
                        {
                            if (replacement == null)
                            {
                                ActiveConnections = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                ActiveConnections = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = ActiveConnections;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.MessagesOut:
                {
                    if (createOrReplace)
                    {
                        if (MessagesOut == null)
                        {
                            if (replacement == null)
                            {
                                MessagesOut = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                MessagesOut = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = MessagesOut;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.MessagesIn:
                {
                    if (createOrReplace)
                    {
                        if (MessagesIn == null)
                        {
                            if (replacement == null)
                            {
                                MessagesIn = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                MessagesIn = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = MessagesIn;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.BytesOut:
                {
                    if (createOrReplace)
                    {
                        if (BytesOut == null)
                        {
                            if (replacement == null)
                            {
                                BytesOut = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                BytesOut = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = BytesOut;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.BytesIn:
                {
                    if (createOrReplace)
                    {
                        if (BytesIn == null)
                        {
                            if (replacement == null)
                            {
                                BytesIn = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                BytesIn = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = BytesIn;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_transport;
        private BaseDataVariableState<uint> m_connectCount;
        private BaseDataVariableState<uint> m_activeConnections;
        private BaseDataVariableState<uint> m_messagesOut;
        private BaseDataVariableState<uint> m_messagesIn;
        private BaseDataVariableState<uint> m_bytesOut;
        private BaseDataVariableState<uint> m_bytesIn;
        #endregion
    }
    #endif
    #endregion

    #region DeviceWatchEventState Class
    #if (!OPCUA_EXCLUDE_DeviceWatchEventState)
    /// <summary>
    /// Stores an instance of the DeviceWatchEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DeviceWatchEventState : BaseEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DeviceWatchEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.DeviceWatchEventType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAcAAAA" +
           "RGV2aWNlV2F0Y2hFdmVudFR5cGVJbnN0YW5jZQEB2wABAdsA/////wkAAAA1YIkKAgAAAAAABwAAAEV2" +
           "ZW50SWQBAdwAAwAAAAArAAAAQSBnbG9iYWxseSB1bmlxdWUgaWRlbnRpZmllciBmb3IgdGhlIGV2ZW50" +
           "LgAuAETcAAAAAA//////AQH/////AAAAADVgiQoCAAAAAAAJAAAARXZlbnRUeXBlAQHdAAMAAAAAIgAA" +
           "AFRoZSBpZGVudGlmaWVyIGZvciB0aGUgZXZlbnQgdHlwZS4ALgBE3QAAAAAR/////wEB/////wAAAAA1" +
           "YIkKAgAAAAAACgAAAFNvdXJjZU5vZGUBAd4AAwAAAAAYAAAAVGhlIHNvdXJjZSBvZiB0aGUgZXZlbnQu" +
           "AC4ARN4AAAAAEf////8BAf////8AAAAANWCJCgIAAAAAAAoAAABTb3VyY2VOYW1lAQHfAAMAAAAAKQAA" +
           "AEEgZGVzY3JpcHRpb24gb2YgdGhlIHNvdXJjZSBvZiB0aGUgZXZlbnQuAC4ARN8AAAAADP////8BAf//" +
           "//8AAAAANWCJCgIAAAAAAAQAAABUaW1lAQHgAAMAAAAAGAAAAFdoZW4gdGhlIGV2ZW50IG9jY3VycmVk" +
           "LgAuAETgAAAAAQAmAf////8BAf////8AAAAANWCJCgIAAAAAAAsAAABSZWNlaXZlVGltZQEB4QADAAAA" +
           "AD4AAABXaGVuIHRoZSBzZXJ2ZXIgcmVjZWl2ZWQgdGhlIGV2ZW50IGZyb20gdGhlIHVuZGVybHlpbmcg" +
           "c3lzdGVtLgAuAEThAAAAAQAmAf////8BAf////8AAAAANWCJCgIAAAAAAAkAAABMb2NhbFRpbWUBAeIA" +
           "AwAAAAA8AAAASW5mb3JtYXRpb24gYWJvdXQgdGhlIGxvY2FsIHRpbWUgd2hlcmUgdGhlIGV2ZW50IG9y" +
           "aWdpbmF0ZWQuAC4AROIAAAABANAi/////wEB/////wAAAAA1YIkKAgAAAAAABwAAAE1lc3NhZ2UBAeMA" +
           "AwAAAAAlAAAAQSBsb2NhbGl6ZWQgZGVzY3JpcHRpb24gb2YgdGhlIGV2ZW50LgAuAETjAAAAABX/////" +
           "AQH/////AAAAADVgiQoCAAAAAAAIAAAAU2V2ZXJpdHkBAeQAAwAAAAAhAAAASW5kaWNhdGVzIGhvdyB1" +
           "cmdlbnQgYW4gZXZlbnQgaXMuAC4AROQAAAAABf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region Iec60870LinkState Class
    #if (!OPCUA_EXCLUDE_Iec60870LinkState)
    /// <summary>
    /// Stores an instance of the Iec60870LinkType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Iec60870LinkState : LinkState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Iec60870LinkState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.Iec60870LinkType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAYAAAA" +
           "SWVjNjA4NzBMaW5rVHlwZUluc3RhbmNlAQEpAQEBKQEBAAAAAQEXAAEBAR4AIQAAABVgiQoCAAAAAQAI" +
           "AAAARGlzYWJsZWQBASoBAC4ARCoBAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAYAAABPbmxpbmUB" +
           "ASsBAC8APysBAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAcAAABFbmFibGVkAQEsAQAvAD8sAQAA" +
           "AAH/////AQH/////AAAAAARhggoEAAAAAQALAAAASW50ZXJyb2dhdGUBAS0BAC8BAesALQEAAAEB////" +
           "/wAAAAAEYYIKBAAAAAEAEgAAAEludGVycm9nYXRlQ2hhbm5lbAEBLgEALwEBFgEuAQAAAQH/////AAAA" +
           "AARhggoEAAAAAQAJAAAAU3luY0Nsb2NrAQEvAQAvAQHsAC8BAAABAf////8AAAAABGGCCgQAAAABAAYA" +
           "AABTZWxlY3QBATABAC8BAQABMAEAAAEB/////wAAAAAEYYIKBAAAAAEABQAAAFdyaXRlAQExAQAvAQEB" +
           "ATEBAAABAf////8AAAAABGGCCgQAAAABAAoAAABXcml0ZVBhcmFtAQEyAQAvAQECATIBAAABAf////8A" +
           "AAAAFWCJCgIAAAABAAkAAABUcmFuc3BvcnQBATMBAC4ARDMBAAAADP////8BAf////8AAAAAFWCJCgIA" +
           "AAABAAwAAABDb25uZWN0Q291bnQBATQBAC8APzQBAAAAB/////8BAf////8AAAAAFWCJCgIAAAABABEA" +
           "AABBY3RpdmVDb25uZWN0aW9ucwEBNQEALwA/NQEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEACwAA" +
           "AE1lc3NhZ2VzT3V0AQE2AQAvAD82AQAAAAf/////AQH/////AAAAABVgiQoCAAAAAQAKAAAATWVzc2Fn" +
           "ZXNJbgEBNwEALwA/NwEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEACAAAAEJ5dGVzT3V0AQE4AQAv" +
           "AD84AQAAAAf/////AQH/////AAAAABVgiQoCAAAAAQAHAAAAQnl0ZXNJbgEBOQEALwA/OQEAAAAH////" +
           "/wEB/////wAAAAAVYIkKAgAAAAEACAAAAFByb3RvY29sAQE6AQAuAEQ6AQAAAAf/////AQH/////AAAA" +
           "ABVgiQoCAAAAAQAEAAAATW9kZQEBOwEALgBEOwEAAAAH/////wEB/////wAAAAAVYKkKAgAAAAEADQAA" +
           "AFNlbmRRdWV1ZVNpemUBATwBAC4ARDwBAAAHDAAAAAAH/////wEB/////wAAAAAVYKkKAgAAAAEAEAAA" +
           "AFJlY2VpdmVRdWV1ZVNpemUBAT0BAC4ARD0BAAAHCAAAAAAH/////wEB/////wAAAAAVYKkKAgAAAAEA" +
           "DgAAAENvbm5lY3RUaW1lb3V0AQE+AQAuAEQ+AQAABx4AAAAAB/////8BAf////8AAAAAFWCpCgIAAAAB" +
           "ABMAAABDb25maXJtYXRpb25UaW1lb3V0AQE/AQAuAEQ/AQAABwUAAAAAB/////8BAf////8AAAAAFWCp" +
           "CgIAAAABABIAAABUZXJtaW5hdGlvblRpbWVvdXQBAUABAC4AREABAAAHFAAAAAAH/////wEB/////wAA" +
           "AAAVYKkKAgAAAAEAEQAAAERldmljZUFkZHJlc3NTaXplAQFBAQAuAERBAQAABwIAAAAAB/////8BAf//" +
           "//8AAAAAFWCpCgIAAAABAAcAAABDT1RTaXplAQFCAQAuAERCAQAABwIAAAAAB/////8BAf////8AAAAA" +
           "FWCpCgIAAAABAA8AAABJbmZvQWRkcmVzc1NpemUBAUMBAC4AREMBAAAHAwAAAAAH/////wEB/////wAA" +
           "AAAVYKkKAgAAAAEADgAAAERhdGFDb2xsZWN0aW9uAQFEAQAuAEREAQAAAQEAAf////8BAf////8AAAAA" +
           "FWCpCgIAAAABAA4AAABTZW5kUmV0cnlDb3VudAEBRQEALgBERQEAAAcAAAAAAAf/////AQH/////AAAA" +
           "ABVgqQoCAAAAAQANAAAAQ1JDUHJvdGVjdGlvbgEBRgEALgBERgEAAAEAAAH/////AQH/////AAAAABVg" +
           "qQoCAAAAAQALAAAAU2VuZFRpbWVvdXQBAUcBAC4AREcBAAAHBQAAAAAH/////wEB/////wAAAAAVYKkK" +
           "AgAAAAEADgAAAFJlY2VpdmVUaW1lb3V0AQFIAQAuAERIAQAABwoAAAAAB/////8BAf////8AAAAAFWCp" +
           "CgIAAAABAAsAAABJZGxlVGltZW91dAEBSQEALgBESQEAAAceAAAAAAf/////AQH/////AAAAABVgqQoC" +
           "AAAAAQANAAAAQW5vbnltb3VzTW9kZQEBSgEALgBESgEAAAEAAAH/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Protocol Property.
        /// </summary>
        public PropertyState<uint> Protocol
        {
            get
            {
                return m_protocol;
            }

            set
            {
                if (!Object.ReferenceEquals(m_protocol, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_protocol = value;
            }
        }

        /// <summary>
        /// A description for the Mode Property.
        /// </summary>
        public PropertyState<uint> Mode
        {
            get
            {
                return m_mode;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mode, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mode = value;
            }
        }

        /// <summary>
        /// A description for the SendQueueSize Property.
        /// </summary>
        public PropertyState<uint> SendQueueSize
        {
            get
            {
                return m_sendQueueSize;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sendQueueSize, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sendQueueSize = value;
            }
        }

        /// <summary>
        /// A description for the ReceiveQueueSize Property.
        /// </summary>
        public PropertyState<uint> ReceiveQueueSize
        {
            get
            {
                return m_receiveQueueSize;
            }

            set
            {
                if (!Object.ReferenceEquals(m_receiveQueueSize, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_receiveQueueSize = value;
            }
        }

        /// <summary>
        /// A description for the ConnectTimeout Property.
        /// </summary>
        public PropertyState<uint> ConnectTimeout
        {
            get
            {
                return m_connectTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_connectTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_connectTimeout = value;
            }
        }

        /// <summary>
        /// A description for the ConfirmationTimeout Property.
        /// </summary>
        public PropertyState<uint> ConfirmationTimeout
        {
            get
            {
                return m_confirmationTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_confirmationTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_confirmationTimeout = value;
            }
        }

        /// <summary>
        /// A description for the TerminationTimeout Property.
        /// </summary>
        public PropertyState<uint> TerminationTimeout
        {
            get
            {
                return m_terminationTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_terminationTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_terminationTimeout = value;
            }
        }

        /// <summary>
        /// A description for the DeviceAddressSize Property.
        /// </summary>
        public PropertyState<uint> DeviceAddressSize
        {
            get
            {
                return m_deviceAddressSize;
            }

            set
            {
                if (!Object.ReferenceEquals(m_deviceAddressSize, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_deviceAddressSize = value;
            }
        }

        /// <summary>
        /// A description for the COTSize Property.
        /// </summary>
        public PropertyState<uint> COTSize
        {
            get
            {
                return m_cOTSize;
            }

            set
            {
                if (!Object.ReferenceEquals(m_cOTSize, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_cOTSize = value;
            }
        }

        /// <summary>
        /// A description for the InfoAddressSize Property.
        /// </summary>
        public PropertyState<uint> InfoAddressSize
        {
            get
            {
                return m_infoAddressSize;
            }

            set
            {
                if (!Object.ReferenceEquals(m_infoAddressSize, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_infoAddressSize = value;
            }
        }

        /// <summary>
        /// A description for the DataCollection Property.
        /// </summary>
        public PropertyState<bool> DataCollection
        {
            get
            {
                return m_dataCollection;
            }

            set
            {
                if (!Object.ReferenceEquals(m_dataCollection, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_dataCollection = value;
            }
        }

        /// <summary>
        /// A description for the SendRetryCount Property.
        /// </summary>
        public PropertyState<uint> SendRetryCount
        {
            get
            {
                return m_sendRetryCount;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sendRetryCount, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sendRetryCount = value;
            }
        }

        /// <summary>
        /// A description for the CRCProtection Property.
        /// </summary>
        public PropertyState<bool> CRCProtection
        {
            get
            {
                return m_cRCProtection;
            }

            set
            {
                if (!Object.ReferenceEquals(m_cRCProtection, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_cRCProtection = value;
            }
        }

        /// <summary>
        /// A description for the SendTimeout Property.
        /// </summary>
        public PropertyState<uint> SendTimeout
        {
            get
            {
                return m_sendTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sendTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sendTimeout = value;
            }
        }

        /// <summary>
        /// A description for the ReceiveTimeout Property.
        /// </summary>
        public PropertyState<uint> ReceiveTimeout
        {
            get
            {
                return m_receiveTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_receiveTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_receiveTimeout = value;
            }
        }

        /// <summary>
        /// A description for the IdleTimeout Property.
        /// </summary>
        public PropertyState<uint> IdleTimeout
        {
            get
            {
                return m_idleTimeout;
            }

            set
            {
                if (!Object.ReferenceEquals(m_idleTimeout, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_idleTimeout = value;
            }
        }

        /// <summary>
        /// A description for the AnonymousMode Property.
        /// </summary>
        public PropertyState<bool> AnonymousMode
        {
            get
            {
                return m_anonymousMode;
            }

            set
            {
                if (!Object.ReferenceEquals(m_anonymousMode, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_anonymousMode = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_protocol != null)
            {
                children.Add(m_protocol);
            }

            if (m_mode != null)
            {
                children.Add(m_mode);
            }

            if (m_sendQueueSize != null)
            {
                children.Add(m_sendQueueSize);
            }

            if (m_receiveQueueSize != null)
            {
                children.Add(m_receiveQueueSize);
            }

            if (m_connectTimeout != null)
            {
                children.Add(m_connectTimeout);
            }

            if (m_confirmationTimeout != null)
            {
                children.Add(m_confirmationTimeout);
            }

            if (m_terminationTimeout != null)
            {
                children.Add(m_terminationTimeout);
            }

            if (m_deviceAddressSize != null)
            {
                children.Add(m_deviceAddressSize);
            }

            if (m_cOTSize != null)
            {
                children.Add(m_cOTSize);
            }

            if (m_infoAddressSize != null)
            {
                children.Add(m_infoAddressSize);
            }

            if (m_dataCollection != null)
            {
                children.Add(m_dataCollection);
            }

            if (m_sendRetryCount != null)
            {
                children.Add(m_sendRetryCount);
            }

            if (m_cRCProtection != null)
            {
                children.Add(m_cRCProtection);
            }

            if (m_sendTimeout != null)
            {
                children.Add(m_sendTimeout);
            }

            if (m_receiveTimeout != null)
            {
                children.Add(m_receiveTimeout);
            }

            if (m_idleTimeout != null)
            {
                children.Add(m_idleTimeout);
            }

            if (m_anonymousMode != null)
            {
                children.Add(m_anonymousMode);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Protocol:
                {
                    if (createOrReplace)
                    {
                        if (Protocol == null)
                        {
                            if (replacement == null)
                            {
                                Protocol = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Protocol = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Protocol;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.Mode:
                {
                    if (createOrReplace)
                    {
                        if (Mode == null)
                        {
                            if (replacement == null)
                            {
                                Mode = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Mode = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Mode;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.SendQueueSize:
                {
                    if (createOrReplace)
                    {
                        if (SendQueueSize == null)
                        {
                            if (replacement == null)
                            {
                                SendQueueSize = new PropertyState<uint>(this);
                            }
                            else
                            {
                                SendQueueSize = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = SendQueueSize;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ReceiveQueueSize:
                {
                    if (createOrReplace)
                    {
                        if (ReceiveQueueSize == null)
                        {
                            if (replacement == null)
                            {
                                ReceiveQueueSize = new PropertyState<uint>(this);
                            }
                            else
                            {
                                ReceiveQueueSize = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = ReceiveQueueSize;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ConnectTimeout:
                {
                    if (createOrReplace)
                    {
                        if (ConnectTimeout == null)
                        {
                            if (replacement == null)
                            {
                                ConnectTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                ConnectTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = ConnectTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ConfirmationTimeout:
                {
                    if (createOrReplace)
                    {
                        if (ConfirmationTimeout == null)
                        {
                            if (replacement == null)
                            {
                                ConfirmationTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                ConfirmationTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = ConfirmationTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.TerminationTimeout:
                {
                    if (createOrReplace)
                    {
                        if (TerminationTimeout == null)
                        {
                            if (replacement == null)
                            {
                                TerminationTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                TerminationTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = TerminationTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.DeviceAddressSize:
                {
                    if (createOrReplace)
                    {
                        if (DeviceAddressSize == null)
                        {
                            if (replacement == null)
                            {
                                DeviceAddressSize = new PropertyState<uint>(this);
                            }
                            else
                            {
                                DeviceAddressSize = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = DeviceAddressSize;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.COTSize:
                {
                    if (createOrReplace)
                    {
                        if (COTSize == null)
                        {
                            if (replacement == null)
                            {
                                COTSize = new PropertyState<uint>(this);
                            }
                            else
                            {
                                COTSize = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = COTSize;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InfoAddressSize:
                {
                    if (createOrReplace)
                    {
                        if (InfoAddressSize == null)
                        {
                            if (replacement == null)
                            {
                                InfoAddressSize = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InfoAddressSize = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InfoAddressSize;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.DataCollection:
                {
                    if (createOrReplace)
                    {
                        if (DataCollection == null)
                        {
                            if (replacement == null)
                            {
                                DataCollection = new PropertyState<bool>(this);
                            }
                            else
                            {
                                DataCollection = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = DataCollection;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.SendRetryCount:
                {
                    if (createOrReplace)
                    {
                        if (SendRetryCount == null)
                        {
                            if (replacement == null)
                            {
                                SendRetryCount = new PropertyState<uint>(this);
                            }
                            else
                            {
                                SendRetryCount = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = SendRetryCount;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.CRCProtection:
                {
                    if (createOrReplace)
                    {
                        if (CRCProtection == null)
                        {
                            if (replacement == null)
                            {
                                CRCProtection = new PropertyState<bool>(this);
                            }
                            else
                            {
                                CRCProtection = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = CRCProtection;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.SendTimeout:
                {
                    if (createOrReplace)
                    {
                        if (SendTimeout == null)
                        {
                            if (replacement == null)
                            {
                                SendTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                SendTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = SendTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ReceiveTimeout:
                {
                    if (createOrReplace)
                    {
                        if (ReceiveTimeout == null)
                        {
                            if (replacement == null)
                            {
                                ReceiveTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                ReceiveTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = ReceiveTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.IdleTimeout:
                {
                    if (createOrReplace)
                    {
                        if (IdleTimeout == null)
                        {
                            if (replacement == null)
                            {
                                IdleTimeout = new PropertyState<uint>(this);
                            }
                            else
                            {
                                IdleTimeout = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = IdleTimeout;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.AnonymousMode:
                {
                    if (createOrReplace)
                    {
                        if (AnonymousMode == null)
                        {
                            if (replacement == null)
                            {
                                AnonymousMode = new PropertyState<bool>(this);
                            }
                            else
                            {
                                AnonymousMode = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = AnonymousMode;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_protocol;
        private PropertyState<uint> m_mode;
        private PropertyState<uint> m_sendQueueSize;
        private PropertyState<uint> m_receiveQueueSize;
        private PropertyState<uint> m_connectTimeout;
        private PropertyState<uint> m_confirmationTimeout;
        private PropertyState<uint> m_terminationTimeout;
        private PropertyState<uint> m_deviceAddressSize;
        private PropertyState<uint> m_cOTSize;
        private PropertyState<uint> m_infoAddressSize;
        private PropertyState<bool> m_dataCollection;
        private PropertyState<uint> m_sendRetryCount;
        private PropertyState<bool> m_cRCProtection;
        private PropertyState<uint> m_sendTimeout;
        private PropertyState<uint> m_receiveTimeout;
        private PropertyState<uint> m_idleTimeout;
        private PropertyState<bool> m_anonymousMode;
        #endregion
    }
    #endif
    #endregion

    #region Iec60870DeviceState Class
    #if (!OPCUA_EXCLUDE_Iec60870DeviceState)
    /// <summary>
    /// Stores an instance of the Iec60870DeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Iec60870DeviceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Iec60870DeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.Iec60870DeviceType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAaAAAA" +
           "SWVjNjA4NzBEZXZpY2VUeXBlSW5zdGFuY2UBAUsBAQFLAQEAAAABARcAAQEBKQEfAAAAFWCJCgIAAAAB" +
           "AAgAAABEaXNhYmxlZAEBTAEALgBETAEAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEABgAAAE9ubGlu" +
           "ZQEBTQEALwA/TQEAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEABwAAAEVuYWJsZWQBAU4BAC8AP04B" +
           "AAAAAf////8BAf////8AAAAABGGCCgQAAAABAAsAAABJbnRlcnJvZ2F0ZQEBTwEALwEB6wBPAQAAAQH/" +
           "////AAAAAARhggoEAAAAAQASAAAASW50ZXJyb2dhdGVDaGFubmVsAQFQAQAvAQEWAVABAAABAf////8A" +
           "AAAABGGCCgQAAAABAAkAAABTeW5jQ2xvY2sBAVEBAC8BAewAUQEAAAEB/////wAAAAAEYYIKBAAAAAEA" +
           "BgAAAFNlbGVjdAEBUgEALwEBAAFSAQAAAQH/////AAAAAARhggoEAAAAAQAFAAAAV3JpdGUBAVMBAC8B" +
           "AQEBUwEAAAEB/////wAAAAAEYYIKBAAAAAEACgAAAFdyaXRlUGFyYW0BAVQBAC8BAQIBVAEAAAEB////" +
           "/wAAAAAVYKkKAgAAAAEABwAAAEFkZHJlc3MBAVUBAC4ARFUBAAAHAQAAAAAH/////wEB/////wAAAAAV" +
           "YKkKAgAAAAEACwAAAExpbmtBZGRyZXNzAQFWAQAuAERWAQAABwEAAAAAB/////8BAf////8AAAAAFWCp" +
           "CgIAAAABABQAAABTdGFydHVwSW50ZXJyb2dhdGlvbgEBVwEALgBEVwEAAAEBAAH/////AQH/////AAAA" +
           "ABVgqQoCAAAAAQATAAAASW50ZXJyb2dhdGlvblBlcmlvZAEBWAEALgBEWAEAAAcAAAAAAAf/////AQH/" +
           "////AAAAABVgqQoCAAAAAQAQAAAAU3RhcnR1cENsb2NrU3luYwEBWQEALgBEWQEAAAEBAAH/////AQH/" +
           "////AAAAABVgqQoCAAAAAQAPAAAAQ2xvY2tTeW5jUGVyaW9kAQFaAQAuAERaAQAABwAAAAAAB/////8B" +
           "Af////8AAAAAFWCJCgIAAAABABkAAABJbnRlcnJvZ2F0aW9uUGVyaW9kR3JvdXAxAQFbAQAuAERbAQAA" +
           "AAf/////AQH/////AAAAABVgiQoCAAAAAQAZAAAASW50ZXJyb2dhdGlvblBlcmlvZEdyb3VwMgEBXAEA" +
           "LgBEXAEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAGQAAAEludGVycm9nYXRpb25QZXJpb2RHcm91" +
           "cDMBAV0BAC4ARF0BAAAAB/////8BAf////8AAAAAFWCJCgIAAAABABkAAABJbnRlcnJvZ2F0aW9uUGVy" +
           "aW9kR3JvdXA0AQFeAQAuAEReAQAAAAf/////AQH/////AAAAABVgiQoCAAAAAQAZAAAASW50ZXJyb2dh" +
           "dGlvblBlcmlvZEdyb3VwNQEBXwEALgBEXwEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAGQAAAElu" +
           "dGVycm9nYXRpb25QZXJpb2RHcm91cDYBAWABAC4ARGABAAAAB/////8BAf////8AAAAAFWCJCgIAAAAB" +
           "ABkAAABJbnRlcnJvZ2F0aW9uUGVyaW9kR3JvdXA3AQFhAQAuAERhAQAAAAf/////AQH/////AAAAABVg" +
           "iQoCAAAAAQAZAAAASW50ZXJyb2dhdGlvblBlcmlvZEdyb3VwOAEBYgEALgBEYgEAAAAH/////wEB////" +
           "/wAAAAAVYIkKAgAAAAEAGQAAAEludGVycm9nYXRpb25QZXJpb2RHcm91cDkBAWMBAC4ARGMBAAAAB///" +
           "//8BAf////8AAAAAFWCJCgIAAAABABoAAABJbnRlcnJvZ2F0aW9uUGVyaW9kR3JvdXAxMAEBZAEALgBE" +
           "ZAEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAGgAAAEludGVycm9nYXRpb25QZXJpb2RHcm91cDEx" +
           "AQFlAQAuAERlAQAAAAf/////AQH/////AAAAABVgiQoCAAAAAQAaAAAASW50ZXJyb2dhdGlvblBlcmlv" +
           "ZEdyb3VwMTIBAWYBAC4ARGYBAAAAB/////8BAf////8AAAAAFWCJCgIAAAABABoAAABJbnRlcnJvZ2F0" +
           "aW9uUGVyaW9kR3JvdXAxMwEBZwEALgBEZwEAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEAGgAAAElu" +
           "dGVycm9nYXRpb25QZXJpb2RHcm91cDE0AQFoAQAuAERoAQAAAAf/////AQH/////AAAAABVgiQoCAAAA" +
           "AQAaAAAASW50ZXJyb2dhdGlvblBlcmlvZEdyb3VwMTUBAWkBAC4ARGkBAAAAB/////8BAf////8AAAAA" +
           "FWCJCgIAAAABABoAAABJbnRlcnJvZ2F0aW9uUGVyaW9kR3JvdXAxNgEBagEALgBEagEAAAAH/////wEB" +
           "/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Address Property.
        /// </summary>
        public PropertyState<uint> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }

        /// <summary>
        /// A description for the LinkAddress Property.
        /// </summary>
        public PropertyState<uint> LinkAddress
        {
            get
            {
                return m_linkAddress;
            }

            set
            {
                if (!Object.ReferenceEquals(m_linkAddress, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_linkAddress = value;
            }
        }

        /// <summary>
        /// A description for the StartupInterrogation Property.
        /// </summary>
        public PropertyState<bool> StartupInterrogation
        {
            get
            {
                return m_startupInterrogation;
            }

            set
            {
                if (!Object.ReferenceEquals(m_startupInterrogation, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_startupInterrogation = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriod Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriod
        {
            get
            {
                return m_interrogationPeriod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriod = value;
            }
        }

        /// <summary>
        /// A description for the StartupClockSync Property.
        /// </summary>
        public PropertyState<bool> StartupClockSync
        {
            get
            {
                return m_startupClockSync;
            }

            set
            {
                if (!Object.ReferenceEquals(m_startupClockSync, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_startupClockSync = value;
            }
        }

        /// <summary>
        /// A description for the ClockSyncPeriod Property.
        /// </summary>
        public PropertyState<uint> ClockSyncPeriod
        {
            get
            {
                return m_clockSyncPeriod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_clockSyncPeriod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_clockSyncPeriod = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup1 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup1
        {
            get
            {
                return m_interrogationPeriodGroup1;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup1, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup1 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup2 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup2
        {
            get
            {
                return m_interrogationPeriodGroup2;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup2, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup2 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup3 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup3
        {
            get
            {
                return m_interrogationPeriodGroup3;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup3, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup3 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup4 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup4
        {
            get
            {
                return m_interrogationPeriodGroup4;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup4, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup4 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup5 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup5
        {
            get
            {
                return m_interrogationPeriodGroup5;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup5, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup5 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup6 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup6
        {
            get
            {
                return m_interrogationPeriodGroup6;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup6, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup6 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup7 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup7
        {
            get
            {
                return m_interrogationPeriodGroup7;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup7, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup7 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup8 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup8
        {
            get
            {
                return m_interrogationPeriodGroup8;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup8, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup8 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup9 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup9
        {
            get
            {
                return m_interrogationPeriodGroup9;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup9, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup9 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup10 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup10
        {
            get
            {
                return m_interrogationPeriodGroup10;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup10, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup10 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup11 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup11
        {
            get
            {
                return m_interrogationPeriodGroup11;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup11, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup11 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup12 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup12
        {
            get
            {
                return m_interrogationPeriodGroup12;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup12, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup12 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup13 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup13
        {
            get
            {
                return m_interrogationPeriodGroup13;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup13, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup13 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup14 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup14
        {
            get
            {
                return m_interrogationPeriodGroup14;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup14, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup14 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup15 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup15
        {
            get
            {
                return m_interrogationPeriodGroup15;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup15, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup15 = value;
            }
        }

        /// <summary>
        /// A description for the InterrogationPeriodGroup16 Property.
        /// </summary>
        public PropertyState<uint> InterrogationPeriodGroup16
        {
            get
            {
                return m_interrogationPeriodGroup16;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interrogationPeriodGroup16, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interrogationPeriodGroup16 = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            if (m_linkAddress != null)
            {
                children.Add(m_linkAddress);
            }

            if (m_startupInterrogation != null)
            {
                children.Add(m_startupInterrogation);
            }

            if (m_interrogationPeriod != null)
            {
                children.Add(m_interrogationPeriod);
            }

            if (m_startupClockSync != null)
            {
                children.Add(m_startupClockSync);
            }

            if (m_clockSyncPeriod != null)
            {
                children.Add(m_clockSyncPeriod);
            }

            if (m_interrogationPeriodGroup1 != null)
            {
                children.Add(m_interrogationPeriodGroup1);
            }

            if (m_interrogationPeriodGroup2 != null)
            {
                children.Add(m_interrogationPeriodGroup2);
            }

            if (m_interrogationPeriodGroup3 != null)
            {
                children.Add(m_interrogationPeriodGroup3);
            }

            if (m_interrogationPeriodGroup4 != null)
            {
                children.Add(m_interrogationPeriodGroup4);
            }

            if (m_interrogationPeriodGroup5 != null)
            {
                children.Add(m_interrogationPeriodGroup5);
            }

            if (m_interrogationPeriodGroup6 != null)
            {
                children.Add(m_interrogationPeriodGroup6);
            }

            if (m_interrogationPeriodGroup7 != null)
            {
                children.Add(m_interrogationPeriodGroup7);
            }

            if (m_interrogationPeriodGroup8 != null)
            {
                children.Add(m_interrogationPeriodGroup8);
            }

            if (m_interrogationPeriodGroup9 != null)
            {
                children.Add(m_interrogationPeriodGroup9);
            }

            if (m_interrogationPeriodGroup10 != null)
            {
                children.Add(m_interrogationPeriodGroup10);
            }

            if (m_interrogationPeriodGroup11 != null)
            {
                children.Add(m_interrogationPeriodGroup11);
            }

            if (m_interrogationPeriodGroup12 != null)
            {
                children.Add(m_interrogationPeriodGroup12);
            }

            if (m_interrogationPeriodGroup13 != null)
            {
                children.Add(m_interrogationPeriodGroup13);
            }

            if (m_interrogationPeriodGroup14 != null)
            {
                children.Add(m_interrogationPeriodGroup14);
            }

            if (m_interrogationPeriodGroup15 != null)
            {
                children.Add(m_interrogationPeriodGroup15);
            }

            if (m_interrogationPeriodGroup16 != null)
            {
                children.Add(m_interrogationPeriodGroup16);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Address = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.LinkAddress:
                {
                    if (createOrReplace)
                    {
                        if (LinkAddress == null)
                        {
                            if (replacement == null)
                            {
                                LinkAddress = new PropertyState<uint>(this);
                            }
                            else
                            {
                                LinkAddress = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = LinkAddress;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.StartupInterrogation:
                {
                    if (createOrReplace)
                    {
                        if (StartupInterrogation == null)
                        {
                            if (replacement == null)
                            {
                                StartupInterrogation = new PropertyState<bool>(this);
                            }
                            else
                            {
                                StartupInterrogation = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = StartupInterrogation;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriod:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriod == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriod = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriod = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriod;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.StartupClockSync:
                {
                    if (createOrReplace)
                    {
                        if (StartupClockSync == null)
                        {
                            if (replacement == null)
                            {
                                StartupClockSync = new PropertyState<bool>(this);
                            }
                            else
                            {
                                StartupClockSync = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = StartupClockSync;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.ClockSyncPeriod:
                {
                    if (createOrReplace)
                    {
                        if (ClockSyncPeriod == null)
                        {
                            if (replacement == null)
                            {
                                ClockSyncPeriod = new PropertyState<uint>(this);
                            }
                            else
                            {
                                ClockSyncPeriod = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = ClockSyncPeriod;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup1:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup1 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup1 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup1 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup1;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup2:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup2 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup2 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup2 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup2;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup3:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup3 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup3 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup3 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup3;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup4:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup4 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup4 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup4 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup4;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup5:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup5 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup5 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup5 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup5;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup6:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup6 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup6 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup6 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup6;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup7:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup7 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup7 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup7 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup7;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup8:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup8 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup8 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup8 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup8;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup9:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup9 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup9 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup9 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup9;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup10:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup10 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup10 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup10 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup10;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup11:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup11 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup11 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup11 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup11;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup12:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup12 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup12 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup12 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup12;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup13:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup13 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup13 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup13 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup13;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup14:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup14 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup14 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup14 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup14;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup15:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup15 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup15 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup15 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup15;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.InterrogationPeriodGroup16:
                {
                    if (createOrReplace)
                    {
                        if (InterrogationPeriodGroup16 == null)
                        {
                            if (replacement == null)
                            {
                                InterrogationPeriodGroup16 = new PropertyState<uint>(this);
                            }
                            else
                            {
                                InterrogationPeriodGroup16 = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = InterrogationPeriodGroup16;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_address;
        private PropertyState<uint> m_linkAddress;
        private PropertyState<bool> m_startupInterrogation;
        private PropertyState<uint> m_interrogationPeriod;
        private PropertyState<bool> m_startupClockSync;
        private PropertyState<uint> m_clockSyncPeriod;
        private PropertyState<uint> m_interrogationPeriodGroup1;
        private PropertyState<uint> m_interrogationPeriodGroup2;
        private PropertyState<uint> m_interrogationPeriodGroup3;
        private PropertyState<uint> m_interrogationPeriodGroup4;
        private PropertyState<uint> m_interrogationPeriodGroup5;
        private PropertyState<uint> m_interrogationPeriodGroup6;
        private PropertyState<uint> m_interrogationPeriodGroup7;
        private PropertyState<uint> m_interrogationPeriodGroup8;
        private PropertyState<uint> m_interrogationPeriodGroup9;
        private PropertyState<uint> m_interrogationPeriodGroup10;
        private PropertyState<uint> m_interrogationPeriodGroup11;
        private PropertyState<uint> m_interrogationPeriodGroup12;
        private PropertyState<uint> m_interrogationPeriodGroup13;
        private PropertyState<uint> m_interrogationPeriodGroup14;
        private PropertyState<uint> m_interrogationPeriodGroup15;
        private PropertyState<uint> m_interrogationPeriodGroup16;
        #endregion
    }
    #endif
    #endregion

    #region ModbusLinkModeType Enumeration
    #if (!OPCUA_EXCLUDE_ModbusLinkModeType)
    /// <summary>
    /// A description for the ModbusLinkModeType DataType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Telecontrol.Scada.Namespaces.Scada)]
    public enum ModbusLinkModeType
    {
        /// <summary>
        /// A description for the Polling field.
        /// </summary>
        [EnumMember(Value = "Polling_1")]
        Polling = 1,

        /// <summary>
        /// A description for the Transmission field.
        /// </summary>
        [EnumMember(Value = "Transmission_2")]
        Transmission = 2,

        /// <summary>
        /// A description for the Listening field.
        /// </summary>
        [EnumMember(Value = "Listening_3")]
        Listening = 3,
    }

    #region ModbusLinkModeTypeCollection Class
    /// <summary>
    /// A collection of ModbusLinkModeType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfModbusLinkModeType", Namespace = Telecontrol.Scada.Namespaces.Scada, ItemName = "ModbusLinkModeType")]
    #if !NET_STANDARD
    public partial class ModbusLinkModeTypeCollection : List<ModbusLinkModeType>, ICloneable
    #else
    public partial class ModbusLinkModeTypeCollection : List<ModbusLinkModeType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ModbusLinkModeTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ModbusLinkModeTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ModbusLinkModeTypeCollection(IEnumerable<ModbusLinkModeType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ModbusLinkModeTypeCollection(ModbusLinkModeType[] values)
        {
            if (values != null)
            {
                return new ModbusLinkModeTypeCollection(values);
            }

            return new ModbusLinkModeTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ModbusLinkModeType[](ModbusLinkModeTypeCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (ModbusLinkModeTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ModbusLinkModeTypeCollection clone = new ModbusLinkModeTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ModbusLinkModeType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ModbusLinkState Class
    #if (!OPCUA_EXCLUDE_ModbusLinkState)
    /// <summary>
    /// Stores an instance of the ModbusLinkType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ModbusLinkState : LinkState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ModbusLinkState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.ModbusLinkType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAWAAAA" +
           "TW9kYnVzTGlua1R5cGVJbnN0YW5jZQEBbAABAWwAAQAAAAEBFwABAQEeABEAAAAVYIkKAgAAAAEACAAA" +
           "AERpc2FibGVkAQHVAAAuAETVAAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAGAAAAT25saW5lAQHW" +
           "AAAvAD/WAAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAHAAAARW5hYmxlZAEB1wAALwA/1wAAAAAB" +
           "/////wEB/////wAAAAAEYYIKBAAAAAEACwAAAEludGVycm9nYXRlAQHzAAAvAQHrAPMAAAABAf////8A" +
           "AAAABGGCCgQAAAABABIAAABJbnRlcnJvZ2F0ZUNoYW5uZWwBARoBAC8BARYBGgEAAAEB/////wAAAAAE" +
           "YYIKBAAAAAEACQAAAFN5bmNDbG9jawEB9AAALwEB7AD0AAAAAQH/////AAAAAARhggoEAAAAAQAGAAAA" +
           "U2VsZWN0AQEMAQAvAQEAAQwBAAABAf////8AAAAABGGCCgQAAAABAAUAAABXcml0ZQEBDQEALwEBAQEN" +
           "AQAAAQH/////AAAAAARhggoEAAAAAQAKAAAAV3JpdGVQYXJhbQEBDgEALwEBAgEOAQAAAQH/////AAAA" +
           "ABVgiQoCAAAAAQAJAAAAVHJhbnNwb3J0AQFuAAAuAERuAAAAAAz/////AQH/////AAAAABVgiQoCAAAA" +
           "AQAMAAAAQ29ubmVjdENvdW50AQFvAAAvAD9vAAAAAAf/////AQH/////AAAAABVgiQoCAAAAAQARAAAA" +
           "QWN0aXZlQ29ubmVjdGlvbnMBAXAAAC8AP3AAAAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAsAAABN" +
           "ZXNzYWdlc091dAEBcQAALwA/cQAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEACgAAAE1lc3NhZ2Vz" +
           "SW4BAXIAAC8AP3IAAAAAB/////8BAf////8AAAAAFWCJCgIAAAABAAgAAABCeXRlc091dAEBcwAALwA/" +
           "cwAAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEABwAAAEJ5dGVzSW4BAXQAAC8AP3QAAAAAB/////8B" +
           "Af////8AAAAAFWCJCgIAAAABAAQAAABNb2RlAQF1AAAuAER1AAAAAQFgAP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Mode Property.
        /// </summary>
        public PropertyState<ModbusLinkModeType> Mode
        {
            get
            {
                return m_mode;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mode, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mode = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_mode != null)
            {
                children.Add(m_mode);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Mode:
                {
                    if (createOrReplace)
                    {
                        if (Mode == null)
                        {
                            if (replacement == null)
                            {
                                Mode = new PropertyState<ModbusLinkModeType>(this);
                            }
                            else
                            {
                                Mode = (PropertyState<ModbusLinkModeType>)replacement;
                            }
                        }
                    }

                    instance = Mode;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<ModbusLinkModeType> m_mode;
        #endregion
    }
    #endif
    #endregion

    #region ModbusDeviceState Class
    #if (!OPCUA_EXCLUDE_ModbusDeviceState)
    /// <summary>
    /// Stores an instance of the ModbusDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ModbusDeviceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ModbusDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.ModbusDeviceType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAYAAAA" +
           "TW9kYnVzRGV2aWNlVHlwZUluc3RhbmNlAQF2AAEBdgABAAAAAQEXAAEBAWwACwAAABVgiQoCAAAAAQAI" +
           "AAAARGlzYWJsZWQBAdgAAC4ARNgAAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAYAAABPbmxpbmUB" +
           "AdkAAC8AP9kAAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAcAAABFbmFibGVkAQHaAAAvAD/aAAAA" +
           "AAH/////AQH/////AAAAAARhggoEAAAAAQALAAAASW50ZXJyb2dhdGUBAfUAAC8BAesA9QAAAAEB////" +
           "/wAAAAAEYYIKBAAAAAEAEgAAAEludGVycm9nYXRlQ2hhbm5lbAEBGwEALwEBFgEbAQAAAQH/////AAAA" +
           "AARhggoEAAAAAQAJAAAAU3luY0Nsb2NrAQH2AAAvAQHsAPYAAAABAf////8AAAAABGGCCgQAAAABAAYA" +
           "AABTZWxlY3QBAQ8BAC8BAQABDwEAAAEB/////wAAAAAEYYIKBAAAAAEABQAAAFdyaXRlAQEQAQAvAQEB" +
           "ARABAAABAf////8AAAAABGGCCgQAAAABAAoAAABXcml0ZVBhcmFtAQERAQAvAQECAREBAAABAf////8A" +
           "AAAAFWCpCgIAAAABAAcAAABBZGRyZXNzAQF4AAAuAER4AAAABwEAAAAAB/////8BAf////8AAAAAFWCp" +
           "CgIAAAABAA4AAABTZW5kUmV0cnlDb3VudAEBeQAALgBEeQAAAAcDAAAAAAf/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the Address Property.
        /// </summary>
        public PropertyState<uint> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }

        /// <summary>
        /// A description for the SendRetryCount Property.
        /// </summary>
        public PropertyState<uint> SendRetryCount
        {
            get
            {
                return m_sendRetryCount;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sendRetryCount, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sendRetryCount = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            if (m_sendRetryCount != null)
            {
                children.Add(m_sendRetryCount);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Address = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }

                case Telecontrol.Scada.BrowseNames.SendRetryCount:
                {
                    if (createOrReplace)
                    {
                        if (SendRetryCount == null)
                        {
                            if (replacement == null)
                            {
                                SendRetryCount = new PropertyState<uint>(this);
                            }
                            else
                            {
                                SendRetryCount = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = SendRetryCount;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_address;
        private PropertyState<uint> m_sendRetryCount;
        #endregion
    }
    #endif
    #endregion

    #region TransmissionItemState Class
    #if (!OPCUA_EXCLUDE_TransmissionItemState)
    /// <summary>
    /// Stores an instance of the TransmissionItemType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransmissionItemState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransmissionItemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Telecontrol.Scada.ObjectTypes.TransmissionItemType, Telecontrol.Scada.Namespaces.Scada, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AQAAACIAAABodHRwOi8vdGVsZWNvbnRyb2wucnUvb3BjLXVhL2Jhc2Uv/////wRggAABAAAAAQAcAAAA" +
           "VHJhbnNtaXNzaW9uSXRlbVR5cGVJbnN0YW5jZQEBIgABASIAAwAAAAEBrAAAAD4BAa0AAAEBHwABARcA" +
           "AQEBIQABAAAAFWCpCgIAAAABAA0AAABTb3VyY2VBZGRyZXNzAQEjAAAuAEQjAAAABwAAAAAAB/////8B" +
           "Af////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <summary>
        /// A description for the SourceAddress Property.
        /// </summary>
        public PropertyState<uint> SourceAddress
        {
            get
            {
                return m_sourceAddress;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sourceAddress, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sourceAddress = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_sourceAddress != null)
            {
                children.Add(m_sourceAddress);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Telecontrol.Scada.BrowseNames.SourceAddress:
                {
                    if (createOrReplace)
                    {
                        if (SourceAddress == null)
                        {
                            if (replacement == null)
                            {
                                SourceAddress = new PropertyState<uint>(this);
                            }
                            else
                            {
                                SourceAddress = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = SourceAddress;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<uint> m_sourceAddress;
        #endregion
    }
    #endif
    #endregion
}