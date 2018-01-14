// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "enums.h"
#include "hantekprotocol/definitions.h"
#include "hantekprotocol/types.h"
#include "hantekprotocol/bulkcode.h"
#include <QList>

namespace Dso {

using namespace Hantek;

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
    ControlSpecification(unsigned channels);
    const ChannelID channels;

    // Interface
    BulkCode cmdSetChannels = BulkCode::INVALID;             ///< Command for setting used channels
    BulkCode cmdSetSamplerate = BulkCode::INVALID;           ///< Command for samplerate settings
    BulkCode cmdSetRecordLength = BulkCode::INVALID;         ///< Command for buffer settings
    BulkCode cmdSetTrigger = BulkCode::INVALID;              ///< Command for trigger settings
    BulkCode cmdSetPretrigger = BulkCode::INVALID;           ///< Command for pretrigger settings
    BulkCode cmdForceTrigger = BulkCode::FORCETRIGGER;       ///< Command for forcing a trigger event
    BulkCode cmdCaptureStart = BulkCode::STARTSAMPLING;      ///< Command for starting the sampling
    BulkCode cmdTriggerEnabled = BulkCode::ENABLETRIGGER;    ///< Command for enabling the trigger
    BulkCode cmdGetData = BulkCode::GETDATA;                 ///< Command for retrieve sample data
    BulkCode cmdGetCaptureState = BulkCode::GETCAPTURESTATE; ///< Command for retrieve the capture state
    BulkCode cmdSetGain = BulkCode::SETGAIN;                 ///< Command for setting the gain

    // Limits
    ControlSpecificationSamplerate samplerate;  ///< The samplerate specifications
    std::vector<RecordLengthID> bufferDividers; ///< Samplerate dividers for record lengths
    unsigned char sampleSize;                   ///< Number of bits per sample

    /// For devices that support only fixed sample rates (isFixedSamplerateDevice=true)
    std::vector<FixedSampleRate> fixedSampleRates;

    // Calibration

    /// The sample values at the top of the screen
    typedef std::vector<unsigned short> VoltageLimit;
    std::vector<VoltageLimit> voltageLimit; // Per channel

    /// Gain levels
    std::vector<ControlSpecificationGainLevel> gain;

    // Features
    std::vector<SpecialTriggerChannel> specialTriggerChannels;
    std::vector<Coupling> couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    std::vector<TriggerMode> triggerModes = {TriggerMode::HARDWARE_SOFTWARE, TriggerMode::WAIT_FORCE,
                                             TriggerMode::SINGLE};
    bool isFixedSamplerateDevice = false;
    bool isSoftwareTriggerDevice = false;
    bool useControlNoBulk = false;
    bool supportsCaptureState = true;
    bool supportsOffset = true;
    bool supportsCouplingRelays = true;
    int fixedUSBinLength = 0;
};
}
