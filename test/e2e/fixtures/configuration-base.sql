--
-- File generated with SQLiteStudio v3.2.1 on Sat Dec 3 14:10:40 2022
--
-- Text encoding used: UTF-8
--
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: AnalogItemType
DROP TABLE IF EXISTS AnalogItemType;
CREATE TABLE "AnalogItemType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Alias TEXT, Severity INTEGER, Input1 TEXT, Input2 TEXT, Locked INTEGER, StalePeriod INTEGER, Output TEXT, OutputTwoStaged INTEGER, OutputCondition TEXT, Simulated INTEGER, HasSimulationSignalNS smallint, HasSimulationSignalID bigint, HasHistoricalDatabaseNS smallint, HasHistoricalDatabaseID bigint, DisplayFormat TEXT, Conversion INTEGER, Clamping INTEGER, EuLo REAL, EuHi REAL, IrLo REAL, IrHi REAL, LimitLo REAL, LimitHi REAL, LimitLoLo REAL, LimitHiHi REAL, EngineeringUnits TEXT, Aperture REAL, Deadband REAL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (1, 4, 1, 'TIT.1', 'Загрузка процессора сервером', 'CPU', 1, '{Server!PCPU}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.##', 0, 0, 0.0, 100.0, 0.0, 1000.0, NULL, NULL, NULL, NULL, '%', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (2, 4, 1, 'TIT.2', 'Использование памяти', 'MEM', 1, '{Server!Mem}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.#', 0, 0, 0.0, 100.0, 0.0, 1000.0, NULL, NULL, NULL, NULL, '%', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (3, 4, 7, 'TIT.3', 'U 110 II сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 2, NULL, NULL, '0.####', 0, 0, 0.0, 100000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (4, 4, 7, 'TIT.4', 'U 35 I сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 3, NULL, NULL, '0.####', 0, 0, 0.0, 100000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (5, 4, 7, 'TIT.5', 'I ВЛ-110 П', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (6, 4, 7, 'TIT.6', 'I МВ-35 Вв Т2', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 5, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (7, 4, 7, 'TIT.7', 'I МВ-35 Вв Т1', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (8, 4, 7, 'TIT.8', 'I ВЛ-110 УНПС', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 2, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (9, 4, 7, 'TIT.9', 'I МВ-10 Вв Т2', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 3, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (10, 4, 7, 'TIT.10', 'I ф.В (БСК-10)', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (11, 4, 7, 'TIT.11', 'U 10 IIсш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 5, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (12, 4, 7, 'TIT.12', 'I ф.С (БСК-10)', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 3, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (13, 4, 7, 'TIT.13', 'I ВЛ-35 МТ', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (14, 4, 7, 'TIT.14', 'U 35 II сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 0, 0, 0.0, 100000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (15, 4, 7, 'TIT.15', 'I ВЛ-35 У', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 5, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (16, 4, 7, 'TIT.16', 'I ф.А (БСК-10)', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (17, 4, 7, 'TIT.17', 'U 10 Iсш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 4, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (18, 4, 7, 'TIT.18', 'I ВЛ-35 ОТ', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 5, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (19, 4, 7, 'TIT.19', 'I МВ-10 Вв Т1', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 3, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (20, 4, 7, 'TIT.20', 'I ВЛ-35 ПО', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, 9, 5, NULL, NULL, '0.####', 1, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (21, 4, 4, 'TIT.21', 'U 35 I сш', 'u35_s1', 1, '{IEC_DEV.1!116}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, 0.0, 10000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (22, 4, 4, 'TIT.22', 'U 35 II сш', 'u35_s2', 1, '{IEC_DEV.1!106}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, 0.0, 100000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (23, 4, 4, 'TIT.23', 'I ВЛ-110 П', 'i_vl110_po', 1, '{IEC_DEV.1!115}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (24, 4, 4, 'TIT.24', 'I ВЛ-110 УНПС', 'i_vl110_unps', 1, '{IEC_DEV.1!112}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (25, 4, 4, 'TIT.25', 'I ВЛ-35 ОТ', 'i_vl35_ot', 1, '{IEC_DEV.1!103}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (26, 4, 4, 'TIT.26', 'I МВ-35 Вв Т2', 'i_mv35_t2', 1, '{IEC_DEV.1!114}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (27, 4, 4, 'TIT.27', 'I ВЛ-35 У', 'i_vl35_u', 1, '{IEC_DEV.1!105}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (28, 4, 4, 'TIT.28', 'I ВЛ-35 МТ', 'i_vl35_mt', 1, '{IEC_DEV.1!107}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (29, 4, 4, 'TIT.29', 'I МВ-35 Вв Т1', 'i_mv35_t1', 1, '{IEC_DEV.1!113}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (30, 4, 4, 'TIT.30', 'U 110 II сш', 'u110_s2', 1, '{IEC_DEV.1!117}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, 0.0, 100000.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (31, 4, 4, 'TIT.31', 'I МВ-10 Вв Т1', 'i_mv10_t1', 1, '{IEC_DEV.1!102}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (32, 4, 4, 'TIT.32', 'I МВ-10 Вв Т2', 'i_mv10_t2', 1, '{IEC_DEV.1!111}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (33, 4, 4, 'TIT.33', 'I ВЛ-35 ПО', 'i_vl35_po', 1, '{IEC_DEV.1!101}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (34, 4, 4, 'TIT.34', 'I ф.В (БСК-10)', 'i_bsk10_b', 1, '{IEC_DEV.1!110}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (35, 4, 4, 'TIT.35', 'U 10 IIсш', 'u10_s2', 1, '{IEC_DEV.1!109}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (36, 4, 4, 'TIT.36', 'I ф.С (БСК-10)', 'i_bsk10_c', 1, '{IEC_DEV.1!108}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (37, 4, 4, 'TIT.37', 'I ф.А (БСК-10)', 'i_bsk10_a', 1, '{IEC_DEV.1!118}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (38, 4, 4, 'TIT.38', 'U 10 Iсш', 'u10_s1', 1, '{IEC_DEV.1!104}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (39, 4, 8, 'TIT.39', 'БСК-10 Сумма фаз', NULL, 0, 'i_bsk10_a + i_bsk10_b + i_bsk10_c', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -300.0, 300.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (40, 4, 4, 'TIT.40', 'I ф.С (БСК-10)', 'i_bsk10_c', 1, '{IEC_DEV.1!108}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);
INSERT INTO AnalogItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, DisplayFormat, Conversion, Clamping, EuLo, EuHi, IrLo, IrHi, LimitLo, LimitHi, LimitLoLo, LimitHiHi, EngineeringUnits, Aperture, Deadband) VALUES (41, 4, 4, 'TIT.41', 'I ф.С (БСК-10)', 'i_bsk10_c', 1, '{IEC_DEV.1!108}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, '0.####', 0, 0, -100.0, 100.0, 0.0, 65535.0, NULL, NULL, NULL, NULL, '?', NULL, NULL);

-- Table: DataGroupType
DROP TABLE IF EXISTS DataGroupType;
CREATE TABLE "DataGroupType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Simulated INTEGER, HasDeviceNS smallint, HasDeviceID bigint);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (1, 7, 24, 'GROUP.1', 'Статистика сервера', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (2, 7, 24, 'GROUP.2', 'Отрадная 110 КВ', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (3, 4, 2, 'GROUP.3', 'ТС', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (4, 4, 2, 'GROUP.4', 'ТИТ', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (5, 7, 24, 'GROUP.5', 'Эмуляция', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (6, 4, 5, 'GROUP.6', 'ТС', 0, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (7, 4, 5, 'GROUP.7', 'ТИТ', 1, NULL, NULL);
INSERT INTO DataGroupType (ID, ParentNS, ParentID, BrowseName, DisplayName, Simulated, HasDeviceNS, HasDeviceID) VALUES (8, 4, 2, 'GROUP.8', 'Дорасчеты', 0, NULL, NULL);

-- Table: DiscreteItemType
DROP TABLE IF EXISTS DiscreteItemType;
CREATE TABLE "DiscreteItemType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Alias TEXT, Severity INTEGER, Input1 TEXT, Input2 TEXT, Locked INTEGER, StalePeriod INTEGER, Output TEXT, OutputTwoStaged INTEGER, OutputCondition TEXT, Simulated INTEGER, HasSimulationSignalNS smallint, HasSimulationSignalID bigint, HasHistoricalDatabaseNS smallint, HasHistoricalDatabaseID bigint, Inverted INTEGER, HasTsFormatNS smallint, HasTsFormatID bigint);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (1, 4, 6, 'TS.1', 'ОД-110 Т2', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (2, 4, 6, 'TS.2', 'МВ-35 Вв Т1', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (3, 4, 6, 'TS.3', 'ОД-110 Т1', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (4, 4, 6, 'TS.4', 'МВ-110 УНПС', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (5, 4, 6, 'TS.5', 'МВ-35 МТ', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (6, 4, 6, 'TS.6', 'МВ-35 ОТ', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (7, 4, 6, 'TS.7', 'МВ-35 У', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (8, 4, 6, 'TS.8', 'МВ-110 П', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (9, 4, 6, 'TS.9', 'МВ-35 ПО', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (10, 4, 6, 'TS.10', 'МВ-10 ВЛ 0-3', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (11, 4, 6, 'TS.11', 'Земля 110 II сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (12, 4, 6, 'TS.12', 'МВ-10 ВЛ 0-9', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (13, 4, 6, 'TS.13', 'МВ-10 ВЛ 0-2', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (14, 4, 6, 'TS.14', 'МВ-10 ВЛ 0-4', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (15, 4, 6, 'TS.15', 'МВ-10 ВЛ 0-8', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (16, 4, 6, 'TS.16', 'МВ-10 ВЛ 0-7', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (17, 4, 6, 'TS.17', 'Земля 35 I сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (18, 4, 6, 'TS.18', 'МВ-10 Вв Т1', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (19, 4, 6, 'TS.19', 'МВ-10 ВЛ 0-6', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (20, 4, 6, 'TS.20', 'Земля 35 II сш', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (21, 4, 6, 'TS.21', 'СМВ-35', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (22, 4, 6, 'TS.22', 'МВ-35 Вв Т2', NULL, 1, NULL, NULL, 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (23, 4, 6, 'TS.23', 'МВ-10 ВЛ О-Резерв', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (24, 4, 6, 'TS.24', 'МВ-10 Вв Т2', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (25, 4, 6, 'TS.25', 'МВ-10 ВЛ 0-5', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (26, 4, 6, 'TS.26', 'СМВ-10', NULL, 1, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (27, 4, 3, 'TS.27', 'МВ-35 У', 'mv35_u', 1, '{IEC_DEV.1!20}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (28, 4, 3, 'TS.28', 'МВ-35 ОТ', 'mv35_ot', 1, '{IEC_DEV.1!21}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (29, 4, 3, 'TS.29', 'МВ-110 П', 'mv110_p', 1, '{IEC_DEV.1!19}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (30, 4, 3, 'TS.30', 'ОД-110 Т2', '?????2', 1, '{IEC_DEV.1!26}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (31, 4, 3, 'TS.31', 'МВ-35 Вв Т1', 'mv35_t1', 1, '{IEC_DEV.1!25}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (32, 4, 3, 'TS.32', 'ОД-110 Т1', '?????1', 1, '{IEC_DEV.1!24}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (33, 4, 3, 'TS.33', 'МВ-110 УНПС', 'mv110_unps', 1, '{IEC_DEV.1!23}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (34, 4, 3, 'TS.34', 'МВ-35 МТ', 'mv35_mt', 1, '{IEC_DEV.1!22}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (35, 4, 3, 'TS.35', 'МВ-35 ПО', 'mv35_po', 1, '{IEC_DEV.1!18}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (36, 4, 3, 'TS.36', 'Земля 110 II сш', '?????5', 1, '{IEC_DEV.1!16}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (37, 4, 3, 'TS.37', 'МВ-10 ВЛ 0-4', 'mv10_o4', 1, '{IEC_DEV.1!13}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (38, 4, 3, 'TS.38', 'МВ-10 ВЛ 0-2', 'mv10_o2', 1, '{IEC_DEV.1!14}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (39, 4, 3, 'TS.39', 'МВ-10 ВЛ 0-9', 'mv10_o9', 1, '{IEC_DEV.1!15}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (40, 4, 3, 'TS.40', 'МВ-10 ВЛ 0-3', 'mv10_o3', 1, '{IEC_DEV.1!17}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (41, 4, 3, 'TS.41', 'Земля 35 II сш', '?????4', 1, '{IEC_DEV.1!7}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (42, 4, 3, 'TS.42', 'МВ-10 Вв Т1', 'mv10_t1', 1, '{IEC_DEV.1!9}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (43, 4, 3, 'TS.43', 'МВ-10 ВЛ 0-5', 'mv10_o5', 1, '{IEC_DEV.1!2}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (44, 4, 3, 'TS.44', 'МВ-35 Вв Т2', 'mv35_t2', 1, '{IEC_DEV.1!3}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (45, 4, 3, 'TS.45', 'СМВ-35', 'smv_35', 1, '{IEC_DEV.1!6}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (46, 4, 3, 'TS.46', 'МВ-10 Вв Т2', 'mv10_t2', 1, '{IEC_DEV.1!5}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (47, 4, 3, 'TS.47', 'СМВ-10', 'smv10', 1, '{IEC_DEV.1!1}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (48, 4, 3, 'TS.48', 'Земля 35 I сш', '?????3', 1, '{IEC_DEV.1!10}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (49, 4, 3, 'TS.49', 'МВ-10 ВЛ 0-7', 'mv10_o7', 1, '{IEC_DEV.1!11}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (50, 4, 3, 'TS.50', 'МВ-10 ВЛ О-Резерв', 'mv10_res', 1, '{IEC_DEV.1!4}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (51, 4, 3, 'TS.51', 'МВ-10 ВЛ 0-8', 'mv10_o8', 1, '{IEC_DEV.1!12}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);
INSERT INTO DiscreteItemType (ID, ParentNS, ParentID, BrowseName, DisplayName, Alias, Severity, Input1, Input2, Locked, StalePeriod, Output, OutputTwoStaged, OutputCondition, Simulated, HasSimulationSignalNS, HasSimulationSignalID, HasHistoricalDatabaseNS, HasHistoricalDatabaseID, Inverted, HasTsFormatNS, HasTsFormatID) VALUES (52, 4, 3, 'TS.52', 'МВ-10 ВЛ 0-6', 'mv10_o6', 1, '{IEC_DEV.1!8}', NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 14, 1);

-- Table: HistoricalDatabaseType
DROP TABLE IF EXISTS HistoricalDatabaseType;
CREATE TABLE "HistoricalDatabaseType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Depth INTEGER);
INSERT INTO HistoricalDatabaseType (ID, ParentNS, ParentID, BrowseName, DisplayName, Depth) VALUES (1, 7, 12, 'HISTORICAL_DB.1', 'ТС (месяц)', 30);
INSERT INTO HistoricalDatabaseType (ID, ParentNS, ParentID, BrowseName, DisplayName, Depth) VALUES (2, 7, 12, 'HISTORICAL_DB.2', 'ТИТ (3 дня)', 3);

-- Table: Iec60870DeviceType
DROP TABLE IF EXISTS Iec60870DeviceType;
CREATE TABLE "Iec60870DeviceType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Disabled INTEGER, HasEventDatabaseNS smallint, HasEventDatabaseID bigint, Address INTEGER, LinkAddress INTEGER, InterrogateOnStart INTEGER, InterrogationPeriod INTEGER, SynchronizeClockOnStart INTEGER, ClockSynchronizationPeriod INTEGER, UtcTime INTEGER, Group1PollPeriod INTEGER, Group2PollPeriod INTEGER, Group3PollPeriod INTEGER, Group4PollPeriod INTEGER, Group5PollPeriod INTEGER, Group6PollPeriod INTEGER, Group7PollPeriod INTEGER, Group8PollPeriod INTEGER, Group9PollPeriod INTEGER, Group10PollPeriod INTEGER, Group11PollPeriod INTEGER, Group12PollPeriod INTEGER, Group13PollPeriod INTEGER, Group14PollPeriod INTEGER, Group15PollPeriod INTEGER, Group16PollPeriod INTEGER);
INSERT INTO Iec60870DeviceType (ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled, HasEventDatabaseNS, HasEventDatabaseID, Address, LinkAddress, InterrogateOnStart, InterrogationPeriod, SynchronizeClockOnStart, ClockSynchronizationPeriod, UtcTime, Group1PollPeriod, Group2PollPeriod, Group3PollPeriod, Group4PollPeriod, Group5PollPeriod, Group6PollPeriod, Group7PollPeriod, Group8PollPeriod, Group9PollPeriod, Group10PollPeriod, Group11PollPeriod, Group12PollPeriod, Group13PollPeriod, Group14PollPeriod, Group15PollPeriod, Group16PollPeriod) VALUES (1, 10, 1, 'IEC_DEV.1', 'Устройство 1', 0, NULL, NULL, 1, 1, 1, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
INSERT INTO Iec60870DeviceType (ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled, HasEventDatabaseNS, HasEventDatabaseID, Address, LinkAddress, InterrogateOnStart, InterrogationPeriod, SynchronizeClockOnStart, ClockSynchronizationPeriod, UtcTime, Group1PollPeriod, Group2PollPeriod, Group3PollPeriod, Group4PollPeriod, Group5PollPeriod, Group6PollPeriod, Group7PollPeriod, Group8PollPeriod, Group9PollPeriod, Group10PollPeriod, Group11PollPeriod, Group12PollPeriod, Group13PollPeriod, Group14PollPeriod, Group15PollPeriod, Group16PollPeriod) VALUES (2, 10, 2, 'IEC_DEV.2', 'Устройство МЭК', 0, NULL, NULL, 1, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- Table: Iec60870LinkType
DROP TABLE IF EXISTS Iec60870LinkType;
CREATE TABLE "Iec60870LinkType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Disabled INTEGER, HasEventDatabaseNS smallint, HasEventDatabaseID bigint, TransportString TEXT, Protocol INTEGER, Mode INTEGER, SendQueueSize INTEGER, ReceiveQueueSize INTEGER, ConnectTimeout INTEGER, ConfirmationTimeout INTEGER, TerminationTimeout INTEGER, DeviceAddressSize INTEGER, CotSize INTEGER, InfoAddressSize INTEGER, CollectData INTEGER, SendRetries INTEGER, CrcProtection INTEGER, SendTimeout INTEGER, ReceiveTimeout INTEGER, IdleTimeout INTEGER, AnonymousMode INTEGER);
INSERT INTO Iec60870LinkType (ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled, HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode, SendQueueSize, ReceiveQueueSize, ConnectTimeout, ConfirmationTimeout, TerminationTimeout, DeviceAddressSize, CotSize, InfoAddressSize, CollectData, SendRetries, CrcProtection, SendTimeout, ReceiveTimeout, IdleTimeout, AnonymousMode) VALUES (1, 7, 30, 'IEC_LINK.1', 'Отрадная МЭК-104', 0, NULL, NULL, 'TCP;Active;Host=127.0.0.1;Port=2404', 0, 0, 12, 8, 30, 5, 30, 2, 2, 3, 1, 0, 0, 10, 5, 30, 0);
INSERT INTO Iec60870LinkType (ID, ParentNS, ParentID, BrowseName, DisplayName, Disabled, HasEventDatabaseNS, HasEventDatabaseID, TransportString, Protocol, Mode, SendQueueSize, ReceiveQueueSize, ConnectTimeout, ConfirmationTimeout, TerminationTimeout, DeviceAddressSize, CotSize, InfoAddressSize, CollectData, SendRetries, CrcProtection, SendTimeout, ReceiveTimeout, IdleTimeout, AnonymousMode) VALUES (2, 7, 30, 'IEC_LINK.2', 'Эмуляция МЭК-104', 0, NULL, NULL, 'TCP;Passive;Host=localhost;Port=2404', 0, 1, 12, 8, 30, 5, 20, 2, 2, 3, 0, 0, 0, 10, 5, 30, 0);

-- Table: Iec60870TransmissionItemType
DROP TABLE IF EXISTS Iec60870TransmissionItemType;
CREATE TABLE Iec60870TransmissionItemType(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, InfoAddress INTEGER, IecTransmitSourceNS smallint, IecTransmitSourceID bigint);

-- Table: Iec61850DeviceType
DROP TABLE IF EXISTS Iec61850DeviceType;
CREATE TABLE "Iec61850DeviceType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Disabled INTEGER, HasEventDatabaseNS smallint, HasEventDatabaseID bigint, Host TEXT, Port INTEGER);

-- Table: Iec61850TransmissionItemType
DROP TABLE IF EXISTS Iec61850TransmissionItemType;
CREATE TABLE Iec61850TransmissionItemType(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, InfoAddress INTEGER, IecTransmitSourceNS smallint, IecTransmitSourceID bigint);

-- Table: ModbusDeviceType
DROP TABLE IF EXISTS ModbusDeviceType;
CREATE TABLE "ModbusDeviceType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Disabled INTEGER, HasEventDatabaseNS smallint, HasEventDatabaseID bigint, Address INTEGER, RepeatCount INTEGER, ResponseTimeout INTEGER);

-- Table: ModbusLinkType
DROP TABLE IF EXISTS ModbusLinkType;
CREATE TABLE "ModbusLinkType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Disabled INTEGER, HasEventDatabaseNS smallint, HasEventDatabaseID bigint, TransportString TEXT, Protocol INTEGER, Mode INTEGER, RequestDelay INTEGER);

-- Table: ModbusTransmissionItemType
DROP TABLE IF EXISTS ModbusTransmissionItemType;
CREATE TABLE ModbusTransmissionItemType(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, InfoAddress INTEGER, IecTransmitSourceNS smallint, IecTransmitSourceID bigint);

-- Table: RCB
DROP TABLE IF EXISTS RCB;
CREATE TABLE "RCB"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Address TEXT);

-- Table: SimulationSignalType
DROP TABLE IF EXISTS SimulationSignalType;
CREATE TABLE "SimulationSignalType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, Type INTEGER, Period INTEGER, Phase INTEGER, UpdateInterval INTEGER);
INSERT INTO SimulationSignalType (ID, ParentNS, ParentID, BrowseName, DisplayName, Type, Period, Phase, UpdateInterval) VALUES (2, 7, 28, 'SIM_ITEM.2', 'Случайный', 0, 60000, 0, 1000);
INSERT INTO SimulationSignalType (ID, ParentNS, ParentID, BrowseName, DisplayName, Type, Period, Phase, UpdateInterval) VALUES (3, 7, 28, 'SIM_ITEM.3', 'Пилообразный', 1, 30000, 0, 1500);
INSERT INTO SimulationSignalType (ID, ParentNS, ParentID, BrowseName, DisplayName, Type, Period, Phase, UpdateInterval) VALUES (4, 7, 28, 'SIM_ITEM.4', 'Синусоидальный', 3, 40000, 0, 500);
INSERT INTO SimulationSignalType (ID, ParentNS, ParentID, BrowseName, DisplayName, Type, Period, Phase, UpdateInterval) VALUES (5, 7, 28, 'SIM_ITEM.5', 'Косинусоидальный', 3, 80000, 0, 2000);

-- Table: TransmissionItemType
DROP TABLE IF EXISTS TransmissionItemType;
CREATE TABLE TransmissionItemType(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, DisplayName TEXT, InfoAddress INTEGER, IecTransmitSourceNS smallint, IecTransmitSourceID bigint, IecTransmitTargetDeviceNS smallint, IecTransmitTargetDeviceID bigint);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (1, 7, 33, NULL, 101, 2, 20, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (2, 7, 33, NULL, 102, 2, 19, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (3, 7, 33, NULL, 103, 2, 18, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (4, 7, 33, NULL, 104, 2, 17, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (6, 7, 33, NULL, 105, 2, 15, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (7, 7, 33, NULL, 106, 2, 14, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (8, 7, 33, NULL, 107, 2, 13, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (9, 7, 33, NULL, 108, 2, 12, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (10, 7, 33, NULL, 109, 2, 11, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (11, 7, 33, NULL, 110, 2, 10, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (12, 7, 33, NULL, 111, 2, 9, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (13, 7, 33, NULL, 112, 2, 8, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (14, 7, 33, NULL, 113, 2, 7, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (15, 7, 33, NULL, 114, 2, 6, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (16, 7, 33, NULL, 115, 2, 5, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (17, 7, 33, NULL, 116, 2, 4, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (18, 7, 33, NULL, 117, 2, 3, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (19, 7, 33, NULL, 118, 2, 16, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (20, 7, 33, NULL, 1, 1, 26, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (21, 7, 33, NULL, 2, 1, 25, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (22, 7, 33, NULL, 3, 1, 24, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (23, 7, 33, NULL, 4, 1, 23, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (24, 7, 33, NULL, 5, 1, 22, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (25, 7, 33, NULL, 6, 1, 21, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (26, 7, 33, NULL, 7, 1, 20, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (27, 7, 33, NULL, 8, 1, 19, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (28, 7, 33, NULL, 9, 1, 18, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (29, 7, 33, NULL, 10, 1, 17, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (30, 7, 33, NULL, 11, 1, 16, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (31, 7, 33, NULL, 12, 1, 15, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (32, 7, 33, NULL, 13, 1, 14, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (33, 7, 33, NULL, 14, 1, 13, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (34, 7, 33, NULL, 15, 1, 12, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (35, 7, 33, NULL, 16, 1, 11, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (36, 7, 33, NULL, 17, 1, 10, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (37, 7, 33, NULL, 18, 1, 9, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (38, 7, 33, NULL, 19, 1, 8, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (39, 7, 33, NULL, 20, 1, 7, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (40, 7, 33, NULL, 21, 1, 6, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (41, 7, 33, NULL, 22, 1, 5, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (42, 7, 33, NULL, 23, 1, 4, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (43, 7, 33, NULL, 24, 1, 3, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (44, 7, 33, NULL, 25, 1, 2, 11, 2);
INSERT INTO TransmissionItemType (ID, ParentNS, ParentID, DisplayName, InfoAddress, IecTransmitSourceNS, IecTransmitSourceID, IecTransmitTargetDeviceNS, IecTransmitTargetDeviceID) VALUES (45, 7, 33, NULL, 26, 1, 1, 11, 2);

-- Table: TsFormatType
DROP TABLE IF EXISTS TsFormatType;
CREATE TABLE "TsFormatType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, OpenLabel TEXT, CloseLabel TEXT, OpenColor INTEGER, CloseColor INTEGER);
INSERT INTO TsFormatType (ID, ParentNS, ParentID, BrowseName, DisplayName, OpenLabel, CloseLabel, OpenColor, CloseColor) VALUES (1, 7, 27, 'TS_PARAMS.1', 'Отключено/Включено', '?????????', '????????', NULL, NULL);

-- Table: UserType
DROP TABLE IF EXISTS UserType;
CREATE TABLE "UserType"(ID INTEGER PRIMARY KEY, ParentNS smallint, ParentID bigint, BrowseName TEXT, DisplayName TEXT, AccessRights INTEGER, MultiSessions INTEGER);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (1, 7, 29, 'Клиент 1', 'Клиент 1', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (2, 7, 29, 'Клиент 2', 'Клиент 2', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (3, 7, 29, 'Клиент 3', 'Клиент 3', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (4, 7, 29, 'Клиент 4', 'Клиент 4', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (5, 7, 29, 'Клиент 5', 'Клиент 5', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (6, 7, 29, 'Клиент 6', 'Клиент 6', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (7, 7, 29, 'Клиент 7', 'Клиент 7', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (8, 7, 29, 'Клиент 8', 'Клиент 8', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (9, 7, 29, 'Клиент 9', 'Клиент 9', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (10, 7, 29, 'Клиент 10', 'Клиент 10', 0, NULL);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (11, 7, 29, 'root', 'root', 3, 0);
INSERT INTO UserType (ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, MultiSessions) VALUES (12, 7, 29, 'guest', 'guest', 0, 1);

-- Index: AnalogItemTypeHasHistoricalDatabaseIndex
DROP INDEX IF EXISTS AnalogItemTypeHasHistoricalDatabaseIndex;
CREATE INDEX AnalogItemTypeHasHistoricalDatabaseIndex ON AnalogItemType(HasHistoricalDatabaseNS, HasHistoricalDatabaseID);

-- Index: AnalogItemTypeHasSimulationSignalIndex
DROP INDEX IF EXISTS AnalogItemTypeHasSimulationSignalIndex;
CREATE INDEX AnalogItemTypeHasSimulationSignalIndex ON AnalogItemType(HasSimulationSignalNS, HasSimulationSignalID);

-- Index: AnalogItemTypeIDIndex
DROP INDEX IF EXISTS AnalogItemTypeIDIndex;
CREATE UNIQUE INDEX AnalogItemTypeIDIndex ON AnalogItemType(ID);

-- Index: AnalogItemTypeParentIndex
DROP INDEX IF EXISTS AnalogItemTypeParentIndex;
CREATE INDEX AnalogItemTypeParentIndex ON AnalogItemType(ParentNS, ParentID);

-- Index: DataGroupTypeHasDeviceIndex
DROP INDEX IF EXISTS DataGroupTypeHasDeviceIndex;
CREATE INDEX DataGroupTypeHasDeviceIndex ON DataGroupType(HasDeviceNS, HasDeviceID);

-- Index: DataGroupTypeIDIndex
DROP INDEX IF EXISTS DataGroupTypeIDIndex;
CREATE UNIQUE INDEX DataGroupTypeIDIndex ON DataGroupType(ID);

-- Index: DataGroupTypeParentIndex
DROP INDEX IF EXISTS DataGroupTypeParentIndex;
CREATE INDEX DataGroupTypeParentIndex ON DataGroupType(ParentNS, ParentID);

-- Index: DiscreteItemTypeHasHistoricalDatabaseIndex
DROP INDEX IF EXISTS DiscreteItemTypeHasHistoricalDatabaseIndex;
CREATE INDEX DiscreteItemTypeHasHistoricalDatabaseIndex ON DiscreteItemType(HasHistoricalDatabaseNS, HasHistoricalDatabaseID);

-- Index: DiscreteItemTypeHasSimulationSignalIndex
DROP INDEX IF EXISTS DiscreteItemTypeHasSimulationSignalIndex;
CREATE INDEX DiscreteItemTypeHasSimulationSignalIndex ON DiscreteItemType(HasSimulationSignalNS, HasSimulationSignalID);

-- Index: DiscreteItemTypeHasTsFormatIndex
DROP INDEX IF EXISTS DiscreteItemTypeHasTsFormatIndex;
CREATE INDEX DiscreteItemTypeHasTsFormatIndex ON DiscreteItemType(HasTsFormatNS, HasTsFormatID);

-- Index: DiscreteItemTypeIDIndex
DROP INDEX IF EXISTS DiscreteItemTypeIDIndex;
CREATE UNIQUE INDEX DiscreteItemTypeIDIndex ON DiscreteItemType(ID);

-- Index: DiscreteItemTypeParentIndex
DROP INDEX IF EXISTS DiscreteItemTypeParentIndex;
CREATE INDEX DiscreteItemTypeParentIndex ON DiscreteItemType(ParentNS, ParentID);

-- Index: HistoricalDatabaseTypeIDIndex
DROP INDEX IF EXISTS HistoricalDatabaseTypeIDIndex;
CREATE UNIQUE INDEX HistoricalDatabaseTypeIDIndex ON HistoricalDatabaseType(ID);

-- Index: HistoricalDatabaseTypeParentIndex
DROP INDEX IF EXISTS HistoricalDatabaseTypeParentIndex;
CREATE INDEX HistoricalDatabaseTypeParentIndex ON HistoricalDatabaseType(ParentNS, ParentID);

-- Index: Iec60870DeviceTypeHasEventDatabaseIndex
DROP INDEX IF EXISTS Iec60870DeviceTypeHasEventDatabaseIndex;
CREATE INDEX Iec60870DeviceTypeHasEventDatabaseIndex ON Iec60870DeviceType(HasEventDatabaseNS, HasEventDatabaseID);

-- Index: Iec60870DeviceTypeIDIndex
DROP INDEX IF EXISTS Iec60870DeviceTypeIDIndex;
CREATE UNIQUE INDEX Iec60870DeviceTypeIDIndex ON Iec60870DeviceType(ID);

-- Index: Iec60870DeviceTypeParentIndex
DROP INDEX IF EXISTS Iec60870DeviceTypeParentIndex;
CREATE INDEX Iec60870DeviceTypeParentIndex ON Iec60870DeviceType(ParentNS, ParentID);

-- Index: Iec60870LinkTypeHasEventDatabaseIndex
DROP INDEX IF EXISTS Iec60870LinkTypeHasEventDatabaseIndex;
CREATE INDEX Iec60870LinkTypeHasEventDatabaseIndex ON Iec60870LinkType(HasEventDatabaseNS, HasEventDatabaseID);

-- Index: Iec60870LinkTypeIDIndex
DROP INDEX IF EXISTS Iec60870LinkTypeIDIndex;
CREATE UNIQUE INDEX Iec60870LinkTypeIDIndex ON Iec60870LinkType(ID);

-- Index: Iec60870LinkTypeParentIndex
DROP INDEX IF EXISTS Iec60870LinkTypeParentIndex;
CREATE INDEX Iec60870LinkTypeParentIndex ON Iec60870LinkType(ParentNS, ParentID);

-- Index: Iec60870TransmissionItemTypeIDIndex
DROP INDEX IF EXISTS Iec60870TransmissionItemTypeIDIndex;
CREATE UNIQUE INDEX Iec60870TransmissionItemTypeIDIndex ON Iec60870TransmissionItemType(ID);

-- Index: Iec60870TransmissionItemTypeIecTransmitSourceIndex
DROP INDEX IF EXISTS Iec60870TransmissionItemTypeIecTransmitSourceIndex;
CREATE INDEX Iec60870TransmissionItemTypeIecTransmitSourceIndex ON Iec60870TransmissionItemType(IecTransmitSourceNS, IecTransmitSourceID);

-- Index: Iec60870TransmissionItemTypeParentIndex
DROP INDEX IF EXISTS Iec60870TransmissionItemTypeParentIndex;
CREATE INDEX Iec60870TransmissionItemTypeParentIndex ON Iec60870TransmissionItemType(ParentNS, ParentID);

-- Index: Iec61850DeviceTypeHasEventDatabaseIndex
DROP INDEX IF EXISTS Iec61850DeviceTypeHasEventDatabaseIndex;
CREATE INDEX Iec61850DeviceTypeHasEventDatabaseIndex ON Iec61850DeviceType(HasEventDatabaseNS, HasEventDatabaseID);

-- Index: Iec61850DeviceTypeIDIndex
DROP INDEX IF EXISTS Iec61850DeviceTypeIDIndex;
CREATE UNIQUE INDEX Iec61850DeviceTypeIDIndex ON Iec61850DeviceType(ID);

-- Index: Iec61850DeviceTypeParentIndex
DROP INDEX IF EXISTS Iec61850DeviceTypeParentIndex;
CREATE INDEX Iec61850DeviceTypeParentIndex ON Iec61850DeviceType(ParentNS, ParentID);

-- Index: Iec61850TransmissionItemTypeIDIndex
DROP INDEX IF EXISTS Iec61850TransmissionItemTypeIDIndex;
CREATE UNIQUE INDEX Iec61850TransmissionItemTypeIDIndex ON Iec61850TransmissionItemType(ID);

-- Index: Iec61850TransmissionItemTypeIecTransmitSourceIndex
DROP INDEX IF EXISTS Iec61850TransmissionItemTypeIecTransmitSourceIndex;
CREATE INDEX Iec61850TransmissionItemTypeIecTransmitSourceIndex ON Iec61850TransmissionItemType(IecTransmitSourceNS, IecTransmitSourceID);

-- Index: Iec61850TransmissionItemTypeParentIndex
DROP INDEX IF EXISTS Iec61850TransmissionItemTypeParentIndex;
CREATE INDEX Iec61850TransmissionItemTypeParentIndex ON Iec61850TransmissionItemType(ParentNS, ParentID);

-- Index: ModbusDeviceTypeHasEventDatabaseIndex
DROP INDEX IF EXISTS ModbusDeviceTypeHasEventDatabaseIndex;
CREATE INDEX ModbusDeviceTypeHasEventDatabaseIndex ON ModbusDeviceType(HasEventDatabaseNS, HasEventDatabaseID);

-- Index: ModbusDeviceTypeIDIndex
DROP INDEX IF EXISTS ModbusDeviceTypeIDIndex;
CREATE UNIQUE INDEX ModbusDeviceTypeIDIndex ON ModbusDeviceType(ID);

-- Index: ModbusDeviceTypeParentIndex
DROP INDEX IF EXISTS ModbusDeviceTypeParentIndex;
CREATE INDEX ModbusDeviceTypeParentIndex ON ModbusDeviceType(ParentNS, ParentID);

-- Index: ModbusLinkTypeHasEventDatabaseIndex
DROP INDEX IF EXISTS ModbusLinkTypeHasEventDatabaseIndex;
CREATE INDEX ModbusLinkTypeHasEventDatabaseIndex ON ModbusLinkType(HasEventDatabaseNS, HasEventDatabaseID);

-- Index: ModbusLinkTypeIDIndex
DROP INDEX IF EXISTS ModbusLinkTypeIDIndex;
CREATE UNIQUE INDEX ModbusLinkTypeIDIndex ON ModbusLinkType(ID);

-- Index: ModbusLinkTypeParentIndex
DROP INDEX IF EXISTS ModbusLinkTypeParentIndex;
CREATE INDEX ModbusLinkTypeParentIndex ON ModbusLinkType(ParentNS, ParentID);

-- Index: ModbusTransmissionItemTypeIDIndex
DROP INDEX IF EXISTS ModbusTransmissionItemTypeIDIndex;
CREATE UNIQUE INDEX ModbusTransmissionItemTypeIDIndex ON ModbusTransmissionItemType(ID);

-- Index: ModbusTransmissionItemTypeIecTransmitSourceIndex
DROP INDEX IF EXISTS ModbusTransmissionItemTypeIecTransmitSourceIndex;
CREATE INDEX ModbusTransmissionItemTypeIecTransmitSourceIndex ON ModbusTransmissionItemType(IecTransmitSourceNS, IecTransmitSourceID);

-- Index: ModbusTransmissionItemTypeParentIndex
DROP INDEX IF EXISTS ModbusTransmissionItemTypeParentIndex;
CREATE INDEX ModbusTransmissionItemTypeParentIndex ON ModbusTransmissionItemType(ParentNS, ParentID);

-- Index: RCBIDIndex
DROP INDEX IF EXISTS RCBIDIndex;
CREATE UNIQUE INDEX RCBIDIndex ON RCB(ID);

-- Index: RCBParentIndex
DROP INDEX IF EXISTS RCBParentIndex;
CREATE INDEX RCBParentIndex ON RCB(ParentNS, ParentID);

-- Index: SimulationSignalTypeIDIndex
DROP INDEX IF EXISTS SimulationSignalTypeIDIndex;
CREATE UNIQUE INDEX SimulationSignalTypeIDIndex ON SimulationSignalType(ID);

-- Index: SimulationSignalTypeParentIndex
DROP INDEX IF EXISTS SimulationSignalTypeParentIndex;
CREATE INDEX SimulationSignalTypeParentIndex ON SimulationSignalType(ParentNS, ParentID);

-- Index: TransmissionItemTypeIDIndex
DROP INDEX IF EXISTS TransmissionItemTypeIDIndex;
CREATE UNIQUE INDEX TransmissionItemTypeIDIndex ON TransmissionItemType(ID);

-- Index: TransmissionItemTypeParentIndex
DROP INDEX IF EXISTS TransmissionItemTypeParentIndex;
CREATE INDEX TransmissionItemTypeParentIndex ON TransmissionItemType(ParentNS, ParentID);

-- Index: TsFormatTypeIDIndex
DROP INDEX IF EXISTS TsFormatTypeIDIndex;
CREATE UNIQUE INDEX TsFormatTypeIDIndex ON TsFormatType(ID);

-- Index: TsFormatTypeParentIndex
DROP INDEX IF EXISTS TsFormatTypeParentIndex;
CREATE INDEX TsFormatTypeParentIndex ON TsFormatType(ParentNS, ParentID);

-- Index: UserTypeIDIndex
DROP INDEX IF EXISTS UserTypeIDIndex;
CREATE UNIQUE INDEX UserTypeIDIndex ON UserType(ID);

-- Index: UserTypeParentIndex
DROP INDEX IF EXISTS UserTypeParentIndex;
CREATE INDEX UserTypeParentIndex ON UserType(ParentNS, ParentID);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
