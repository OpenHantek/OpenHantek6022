// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "enums.h"
#include "hantekprotocol/bulkcode.h"
#include "hantekprotocol/controlcode.h"
#include "hantekprotocol/controlvalue.h"
#include "hantekprotocol/definitions.h"
#include <QList>

namespace Dso {

using namespace Hantek;

/// \brief Stores the bulk command codes used for this device.
struct ControlSpecificationCommandsBulk {
    BulkCode setChannels = BulkCode::INVALID;     ///< Command for setting used channels
    BulkCode setSamplerate = BulkCode::INVALID;   ///< Command for samplerate settings
    BulkCode setGain = BulkCode::SETGAIN;         ///< Command for gain settings (Usually in combination with
                                                  /// CONTROL_SETRELAYS)
    BulkCode setRecordLength = BulkCode::INVALID; ///< Command for buffer settings
    BulkCode setTrigger = BulkCode::INVALID;      ///< Command for trigger settings
    BulkCode setPretrigger = BulkCode::INVALID;   ///< Command for pretrigger settings
};

/// \brief Stores the command codes used for this device.
struct ControlSpecificationCommands {
    ControlSpecificationCommandsBulk bulk; ///< The used bulk commands
};

/// \brief Stores the samplerate limits for calculations.
struct ControlSamplerateLimits {
    double base;                         ///< The base for sample rate calculations
    double max;                          ///< The maximum sample rate
    unsigned int maxDownsampler;         ///< The maximum downsampling ratio
    std::vector<unsigned> recordLengths; ///< Available record lengths, UINT_MAX means rolling
};

/// \brief Stores the samplerate limits.
struct ControlSpecificationSamplerate {
    ControlSamplerateLimits single = {50e6, 50e6, 0, std::vector<unsigned>()};  ///< The limits for single channel mode
    ControlSamplerateLimits multi = {100e6, 100e6, 0, std::vector<unsigned>()}; ///< The limits for multi channel mode
};

struct ControlSpecificationGainLevel {
    /// The index of the selected gain on the hardware
    unsigned char gainIndex;
    /// Available voltage steps in V/screenheight
    double gainSteps;
};

struct FixedSampleRate {
    unsigned char id;
    double samplerate;
};

struct SpecialTriggerChannel {
    std::string name;
    int hardwareID;
};

/// \brief Stores the specifications of the currently connected device.
struct ControlSpecification {
    ChannelID channels = HANTEK_CHANNELS;

    // Interface
    ControlSpecificationCommands command; ///< The commands for this device

    // Limits
    ControlSpecificationSamplerate samplerate;  ///< The samplerate specifications
    std::vector<RecordLengthID> bufferDividers; ///< Samplerate dividers for record lengths
    unsigned char sampleSize;                   ///< Number of bits per sample

    // Calibration
    /// The sample values at the top of the screen
    std::vector<unsigned short> voltageLimit[HANTEK_CHANNELS];
    /// Calibration data for the channel offsets
    OffsetsPerGainStep offsetLimit[HANTEK_CHANNELS];

    /// Gain levels
    std::vector<ControlSpecificationGainLevel> gain;

    /// For devices that support only fixed sample rates (isFixedSamplerateDevice=true)
    std::vector<FixedSampleRate> fixedSampleRates;

    std::vector<SpecialTriggerChannel> specialTriggerChannels;
    std::vector<Coupling> couplings = {Dso::Coupling::DC, Dso::Coupling::AC};

    bool isFixedSamplerateDevice = false;
    bool isSoftwareTriggerDevice = false;
    bool useControlNoBulk = false;
    bool supportsCaptureState = true;
    bool supportsOffset = true;
    bool supportsCouplingRelays = true;
};
}
