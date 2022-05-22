#pragma once

namespace cfg {

enum class ModbusEncoding : int { RTU, ASCII, TCP };

inline const int NUM_CHANNELS = 2;

enum class Iec60870Protocol : int { IEC104, IEC101 };
enum class Iec60870Mode : int { MASTER, SLAVE, SLAVE_LISTEN };

enum class SimulationSignalType : int { RANDOM = 0, RAMP = 1, STEP = 2, SIN = 3, COS = 4 };

inline const char kDataGroupDevicePlaceholder[] = "GROUP_DEVICE";

} // namespace cfg
