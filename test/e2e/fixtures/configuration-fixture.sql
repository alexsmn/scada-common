BEGIN TRANSACTION;

-- Client e2e overlay for the generated server configuration database.
-- Keep this file client-owned: do not reference other repository fixtures here.

DELETE FROM Iec60870DeviceType;
DELETE FROM Iec60870LinkType;
DELETE FROM Iec60870TransmissionItemType;
DELETE FROM Iec61850DeviceType;
DELETE FROM Iec61850TransmissionItemType;
DELETE FROM ModbusDeviceType;
DELETE FROM ModbusLinkType;
DELETE FROM ModbusTransmissionItemType;

-- MODBUS client/server pair.
INSERT INTO ModbusLinkType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode,
  RequestDelay
) VALUES (
  1, 7, 30, 'E2E_MODBUS_SERVER_LINK', 'E2E MODBUS Server Link', 0,
  NULL, NULL, 'TCP;Passive;Host=127.0.0.1;Port=1502', 2, 1,
  0
);

INSERT INTO ModbusLinkType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode,
  RequestDelay
) VALUES (
  2, 7, 30, 'E2E_MODBUS_CLIENT_LINK', 'E2E MODBUS Client Link', 0,
  NULL, NULL, 'TCP;Active;Host=127.0.0.1;Port=1502', 2, 0,
  0
);

INSERT INTO ModbusDeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Address, RepeatCount, ResponseTimeout
) VALUES (
  1, 12, 1, 'E2E_MODBUS_SERVER_DEVICE', 'E2E MODBUS Server Device', 0,
  NULL, NULL, 1, 0, 1000
);

INSERT INTO ModbusDeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Address, RepeatCount, ResponseTimeout
) VALUES (
  2, 12, 2, 'E2E_MODBUS_CLIENT_DEVICE', 'E2E MODBUS Client Device', 0,
  NULL, NULL, 1, 0, 1000
);

INSERT INTO ModbusTransmissionItemType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, InfoAddress,
  IecTransmitSourceNS, IecTransmitSourceID
) VALUES (
  1, 3, 1, 'E2E_MODBUS_TX_1', 'E2E MODBUS TX 1', 1,
  1, 26
);

-- IEC60870 client/server pair.
INSERT INTO Iec60870LinkType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode,
  SendQueueSize, ReceiveQueueSize, ConnectTimeout, ConfirmationTimeout,
  TerminationTimeout, DeviceAddressSize, CotSize, InfoAddressSize, CollectData,
  SendRetries, CrcProtection, SendTimeout, ReceiveTimeout, IdleTimeout,
  AnonymousMode
) VALUES (
  1, 7, 30, 'E2E_IEC60870_SERVER_LINK', 'E2E IEC60870 Server Link', 0,
  NULL, NULL, 'TCP;Passive;Host=127.0.0.1;Port=2404', 0, 1,
  12, 8, 1, 1, 30, 2, 2, 3, 1,
  0, 0, 10, 5, 1,
  0
);

INSERT INTO Iec60870LinkType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode,
  SendQueueSize, ReceiveQueueSize, ConnectTimeout, ConfirmationTimeout,
  TerminationTimeout, DeviceAddressSize, CotSize, InfoAddressSize, CollectData,
  SendRetries, CrcProtection, SendTimeout, ReceiveTimeout, IdleTimeout,
  AnonymousMode
) VALUES (
  2, 7, 30, 'E2E_IEC60870_CLIENT_LINK', 'E2E IEC60870 Client Link', 0,
  NULL, NULL, 'TCP;Active;Host=127.0.0.1;Port=2404', 0, 0,
  12, 8, 1, 1, 30, 2, 2, 3, 0,
  0, 0, 10, 5, 1,
  0
);

INSERT INTO Iec60870DeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Address, LinkAddress,
  InterrogateOnStart, InterrogationPeriod, SynchronizeClockOnStart,
  ClockSynchronizationPeriod, UtcTime, Group1PollPeriod, Group2PollPeriod,
  Group3PollPeriod, Group4PollPeriod, Group5PollPeriod, Group6PollPeriod,
  Group7PollPeriod, Group8PollPeriod, Group9PollPeriod, Group10PollPeriod,
  Group11PollPeriod, Group12PollPeriod, Group13PollPeriod, Group14PollPeriod,
  Group15PollPeriod, Group16PollPeriod
) VALUES (
  1, 10, 1, 'E2E_IEC60870_SERVER_DEVICE', 'E2E IEC60870 Server Device', 0,
  NULL, NULL, 1, 1,
  0, 0, 0,
  0, NULL, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0
);

INSERT INTO Iec60870DeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Address, LinkAddress,
  InterrogateOnStart, InterrogationPeriod, SynchronizeClockOnStart,
  ClockSynchronizationPeriod, UtcTime, Group1PollPeriod, Group2PollPeriod,
  Group3PollPeriod, Group4PollPeriod, Group5PollPeriod, Group6PollPeriod,
  Group7PollPeriod, Group8PollPeriod, Group9PollPeriod, Group10PollPeriod,
  Group11PollPeriod, Group12PollPeriod, Group13PollPeriod, Group14PollPeriod,
  Group15PollPeriod, Group16PollPeriod
) VALUES (
  2, 10, 2, 'E2E_IEC60870_CLIENT_DEVICE', 'E2E IEC60870 Client Device', 0,
  NULL, NULL, 1, 1,
  1, 1, 0,
  0, NULL, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0
);

INSERT INTO Iec60870TransmissionItemType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, InfoAddress,
  IecTransmitSourceNS, IecTransmitSourceID
) VALUES (
  1, 11, 1, 'E2E_IEC60870_TX_111', 'E2E IEC60870 TX 111', 111,
  1, 26
);

-- IEC61850 has device endpoints only; there is no separate link table.
INSERT INTO Iec61850DeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Host, Port
) VALUES (
  1, 7, 30, 'E2E_IEC61850_CLIENT_DEVICE', 'E2E IEC61850 Client Device', 0,
  NULL, NULL, '127.0.0.1', 102
);

INSERT INTO Iec61850DeviceType (
  ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled,
  HasEventDatabaseNS, HasEventDatabaseID, Host, Port
) VALUES (
  2, 7, 30, 'E2E_IEC61850_SERVER_DEVICE', 'E2E IEC61850 Server Device', 0,
  NULL, NULL, '127.0.0.1', 102
);

COMMIT;
