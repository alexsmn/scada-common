#pragma once

namespace cfg {

enum class ModbusEncoding : int { RTU, ASCII, TCP };

const int NUM_CHANNELS = 2;

enum class Iec60870Protocol : int { IEC104, IEC101 };
enum class Iec60870Mode : int { MASTER, SLAVE, SLAVE_LISTEN };

// special values for Item::Channel::item
const unsigned RID_SERVICE	= -1000;
const unsigned RID_LINK		= RID_SERVICE + 0;	// link state

// service information object addresses
const unsigned RID_IEC60870_FIRST	= RID_SERVICE + 100;
const unsigned RID_IEC60870_RECON	= RID_IEC60870_FIRST + 0;
const unsigned RID_IEC60870_RECV	= RID_IEC60870_FIRST + 1;
const unsigned RID_IEC60870_SENT	= RID_IEC60870_FIRST + 2;

enum class SimulationSignalType : int { RANDOM = 0, RAMP = 1, STEP = 2, SIN = 3, COS = 4 };

} // namespace cfg
