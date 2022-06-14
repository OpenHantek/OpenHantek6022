// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMetaType>
#include <QString>
#include <stdint.h>

#define HANTEK_GAIN_STEPS 8
#define HANTEK_CHANNEL_NUMBER 2

namespace Hantek {
/// \enum UsedChannels
/// \brief The enabled channels.
enum class UsedChannels : uint8_t {
    USED_NONE,   ///< No channels are activated
    USED_CH1,    ///< Only channel 1 is activated
    USED_CH2,    ///< Only channel 2 is activated
    USED_CH1CH2, ///< Channel 1 and 2 are both activated
};

#pragma pack( push, 1 )

// The strct reflects the offset layout in EEPROM
// All values are offset binariy bytes (0 -> 0x80)
// The 1st 32 bytes are already factory calibration values
// 16 byte offset at low speed:  CH0@20mV, CH1@20mV, CH0@50mV,...,CH1@5V
// 16 byte offset at high speed: CH0@20mV, CH1@20mV, CH0@50mV,...,CH1@5V
// The next 16 bytes are written by python calibration tool
// 16 byte gain: CH0@20mV, CH1@20mV, CH0@50mV,...,CH1@5V
// The next 32 bytes are an optional fractional part of offset (*250)
// 16 byte offset (ls) fractional part: CH0@20mV, CH1@20mV, CH0@50mV,...,CH1@5V
// 16 byte offset (hs) fractional part: CH0@20mV, CH1@20mV, CH0@50mV,...,CH1@5V

struct Steps {
    uint8_t step[ HANTEK_GAIN_STEPS ][ HANTEK_CHANNEL_NUMBER ];
};

struct Offsets {
    Steps ls;
    Steps hs;
};

struct CalibrationValues {
    Offsets off;
    Steps gain;
    Offsets fine;
};

#pragma pack( pop )

} // namespace Hantek
