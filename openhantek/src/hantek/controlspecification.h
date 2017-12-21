// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "definitions.h"
#include <QList>

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsBulk                  hantek/control.h
/// \brief Stores the bulk command codes used for this device.
struct ControlSpecificationCommandsBulk {
    BulkCode setChannels;     ///< Command for setting used channels
    BulkCode setSamplerate;   ///< Command for samplerate settings
    BulkCode setGain;         ///< Command for gain settings (Usually in combination with
                              /// CONTROL_SETRELAYS)
    BulkCode setRecordLength; ///< Command for buffer settings
    BulkCode setTrigger;      ///< Command for trigger settings
    BulkCode setPretrigger;   ///< Command for pretrigger settings
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsControl               hantek/control.h
/// \brief Stores the control command codes used for this device.
struct ControlSpecificationCommandsControl {
    ControlCode setOffset; ///< Command for setting offset calibration data
    ControlCode setRelays; ///< Command for setting gain relays (Usually in
                           /// combination with BULK_SETGAIN)
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsValues                hantek/control.h
/// \brief Stores the control value codes used for this device.
struct ControlSpecificationCommandsValues {
    ControlValue offsetLimits;  ///< Code for channel offset limits
    ControlValue voltageLimits; ///< Code for voltage limits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommands                      hantek/control.h
/// \brief Stores the command codes used for this device.
struct ControlSpecificationCommands {
    ControlSpecificationCommandsBulk bulk;       ///< The used bulk commands
    ControlSpecificationCommandsControl control; ///< The used control commands
    ControlSpecificationCommandsValues values;   ///< The used control values
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSamplerateLimits                           hantek/control.h
/// \brief Stores the samplerate limits for calculations.
struct ControlSamplerateLimits {
    double base;                         ///< The base for sample rate calculations
    double max;                          ///< The maximum sample rate
    unsigned int maxDownsampler;         ///< The maximum downsampling ratio
    std::vector<unsigned> recordLengths; ///< Available record lengths, UINT_MAX means rolling
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationSamplerate                    hantek/control.h
/// \brief Stores the samplerate limits.
struct ControlSpecificationSamplerate {
    ControlSamplerateLimits single; ///< The limits for single channel mode
    ControlSamplerateLimits multi;  ///< The limits for multi channel mode
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecification                              hantek/control.h
/// \brief Stores the specifications of the currently connected device.
struct ControlSpecification {
    // Interface
    ControlSpecificationCommands command; ///< The commands for this device

    // Limits
    ControlSpecificationSamplerate samplerate; ///< The samplerate specifications
    QList<unsigned int> bufferDividers;        ///< Samplerate dividers for record lengths
    QList<double> gainSteps;                   ///< Available voltage steps in V/screenheight
    unsigned char sampleSize;                  ///< Number of bits per sample

    // Calibration
    /// The sample values at the top of the screen
    QList<unsigned short> voltageLimit[HANTEK_CHANNELS];
    /// The index of the selected gain on the hardware
    QList<unsigned char> gainIndex;
    QList<unsigned char> gainDiv;
    QList<double> sampleSteps; ///< Available samplerate steps in s
    QList<unsigned char> sampleDiv;
    /// Calibration data for the channel offsets \todo Should probably be a QList
    /// too
    unsigned short offsetLimit[HANTEK_CHANNELS][9][OFFSET_COUNT];

    bool isSoftwareTriggerDevice = false;
    bool useControlNoBulk = false;
    bool supportsCaptureState = true;
    bool supportsOffset = true;
    bool supportsCouplingRelays = true;
};

}

