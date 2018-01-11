// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaType>
#include <QString>
#include <stdint.h>

#define HANTEK_GAIN_STEPS 9

namespace Hantek {
/// \enum UsedChannels
/// \brief The enabled channels.
enum class UsedChannels : uint8_t {
    USED_CH1,    ///< Only channel 1 is activated
    USED_CH2,    ///< Only channel 2 is activated
    USED_CH1CH2, ///< Channel 1 and 2 are both activated
    USED_NONE,    ///< No channels are activated

    // The enabled channels for the DSO-2250.
    BUSED_CH1 = USED_CH1,
    BUSED_NONE = USED_CH2,
    BUSED_CH1CH2 = USED_CH1CH2,
    BUSED_CH2 = USED_NONE
};

/// \enum DTriggerPositionUsed                                  hantek/types.h
/// \brief The trigger position states for the 0x0d command.
enum class DTriggerPositionUsed: uint8_t {
    OFF = 0, ///< Used for Roll mode
    ON = 7   ///< Used for normal operation
};

#pragma pack(push, 1)
struct Offset {
    unsigned short start = 0x0000;
    unsigned short end = 0xffff;
};

struct OffsetsPerGainStep {
    Offset step[HANTEK_GAIN_STEPS];
};

/// \struct FilterBits
/// \brief The bits for BULK::SETFILTER.
struct FilterBits {
    uint8_t channel1 : 1; ///< Set to true when channel 1 isn't used
    uint8_t channel2 : 1; ///< Set to true when channel 2 isn't used
    uint8_t trigger : 1;  ///< Set to true when trigger isn't used
    uint8_t reserved : 5; ///< Unused bits
};


/// \struct GainBits
/// \brief The gain bits for BulkCode::SETGAIN.
struct GainBits {
    uint8_t channel1 : 2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
    uint8_t channel2 : 2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
    uint8_t reserved : 4; ///< Unused bits
};


/// \struct Tsr1Bits
/// \brief Trigger and samplerate bits (Byte 1).
struct Tsr1Bits {
    uint8_t triggerSource : 2;    ///< The trigger source, see Hantek::TriggerSource
    uint8_t recordLength : 3;     ///< See ::RecordLengthId
    uint8_t samplerateId : 2;     ///< Samplerate ID when downsampler is disabled
    uint8_t downsamplingMode : 1; ///< true, if Downsampler is used
};


/// \struct Tsr2Bits
/// \brief Trigger and samplerate bits (Byte 2).
struct Tsr2Bits {
    uint8_t usedChannels : 2; ///< Used channels, see Hantek::UsedChannels
    uint8_t fastRate : 1;     ///< true, if one channels uses all buffers
    uint8_t triggerSlope : 1; ///< The trigger slope, see Dso::Slope, inverted
    /// when Tsr1Bits.samplerateFast is uneven
    uint8_t reserved : 4; ///< Unused bits
};


/// \struct CTriggerBits
/// \brief Trigger bits for 0x0c command.
struct CTriggerBits {
    uint8_t triggerSource : 2; ///< The trigger source, see Hantek::TriggerSource
    uint8_t triggerSlope : 1;  ///< The trigger slope, see Dso::Slope
    uint8_t reserved : 5;      ///< Unused bits
};


/// \struct DBufferBits
/// \brief Buffer mode bits for 0x0d command.
struct DBufferBits {
    uint8_t triggerPositionUsed : 3; ///< See ::DTriggerPositionUsed
    uint8_t recordLength : 3;        ///< See ::RecordLengthId
    uint8_t reserved : 2;            ///< Unused bits
};


/// \struct ESamplerateBits
/// \brief Samplerate bits for DSO-2250 0x0e command.
struct ESamplerateBits {
    uint8_t fastRate : 1;     ///< false, if one channels uses all buffers
    uint8_t downsampling : 1; ///< true, if the downsampler is activated
    uint8_t reserved : 4;     ///< Unused bits
};


/// \struct ETsrBits
/// \brief Trigger and samplerate bits for DSO-5200/DSO-5200A 0x0e command.
struct ETsrBits {
    uint8_t fastRate : 1;      ///< false, if one channels uses all buffers
    uint8_t usedChannels : 2;  ///< Used channels, see Hantek::UsedChannels
    uint8_t triggerSource : 2; ///< The trigger source, see Hantek::TriggerSource
    uint8_t triggerSlope : 2;  ///< The trigger slope, see Dso::Slope
    uint8_t triggerPulse : 1;  ///< Pulses are causing trigger events
};

#pragma pack(pop)

}
