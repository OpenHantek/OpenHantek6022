// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "enums.h"
#include "hantekprotocol/definitions.h"
#include "hantekprotocol/types.h"
#include <QList>

namespace Dso {

using namespace Hantek;

/// \brief Stores the samplerate limits for calculations.
struct ControlSamplerateLimits {
    double base;                           ///< The base for sample rate calculations
    double max;                            ///< The maximum sample rate
    std::vector< unsigned > recordLengths; ///< Available record lengths, UINT_MAX means rolling
};

/// \brief Stores the samplerate limits.
struct ControlSpecificationSamplerate {
    ControlSamplerateLimits single = { 50e6, 50e6, std::vector< unsigned >() };  ///< The limits for single channel mode
    ControlSamplerateLimits multi = { 100e6, 100e6, std::vector< unsigned >() }; ///< The limits for multi channel mode
};

struct ControlSpecificationGainLevel {
    /// The index of the selected gain on the hardware
    unsigned char gainValue; // 1, 2, 5, 10, ..
    /// Available voltage steps in V/div
    double Vdiv;
    // double gainError;
};

struct FixedSampleRate {
    double samplerate;
    unsigned char id;
    unsigned oversampling;
};

/// \brief Stores the specifications of the currently connected device.
struct ControlSpecification {
    explicit ControlSpecification( unsigned channels );
    const ChannelID channels;

    // Limits
    ControlSpecificationSamplerate samplerate; ///< The samplerate specifications

    /// For devices that support only fixed sample rates
    std::vector< FixedSampleRate > fixedSampleRates;

    // Calibration
    /// DSO6022 has calibration in small EEPROM, DDS120 has big fw EEPROM
    bool hasCalibrationEEPROM = true;
    bool isDemoDevice = false;
    /// The sample values for one div
    typedef std::vector< double > VoltageScale;
    std::vector< VoltageScale > voltageScale; // Per channel

    /// Gain levels
    std::vector< ControlSpecificationGainLevel > gain;

    // Features
    std::vector< Coupling > couplings = { Dso::Coupling::DC, Dso::Coupling::AC };
    std::vector< TriggerMode > triggerModes = { TriggerMode::ROLL, TriggerMode::AUTO, TriggerMode::NORMAL, TriggerMode::SINGLE };
    int fixedUSBinLength = 0;
    bool hasACcoupling = false;

    QList< double > calfreqSteps;
};
} // namespace Dso
