// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "enums.h"
#include "hantekprotocol/definitions.h"
#include "hantekprotocol/types.h"
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
    double samplerate;
    unsigned char id;
    unsigned downsampling;
};

/// \brief Stores the specifications of the currently connected device.
struct ControlSpecification {
    explicit ControlSpecification( unsigned channels );
    const ChannelID channels;

    // Limits
    ControlSpecificationSamplerate samplerate;  ///< The samplerate specifications
    std::vector<RecordLengthID> bufferDividers; ///< Samplerate dividers for record lengths

    /// For devices that support only fixed sample rates
    std::vector<FixedSampleRate> fixedSampleRates;

    // Calibration
    /// DSO6022 has calibration in small EEPROM, DDS120 has big fw EEPROM
    bool hasCalibrationStorage = true;
    /// The sample values at the top of the screen
    typedef std::vector<int> VoltageScale;
    std::vector<VoltageScale> voltageScale; // Per channel
    typedef std::vector<int> VoltageOffset;
    std::vector<VoltageOffset> voltageOffset; // Per channel

    /// Gain levels
    std::vector<ControlSpecificationGainLevel> gain;

    // Features
    std::vector<Coupling> couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    std::vector<TriggerMode> triggerModes = {TriggerMode::AUTO, TriggerMode::NORMAL,
                                             TriggerMode::SINGLE};
    int fixedUSBinLength = 0;

    QList<double> calfreqSteps;
};
}
