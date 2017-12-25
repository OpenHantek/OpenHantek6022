// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "hantekprotocol/controlvalue.h"
#include "hantekprotocol/bulkcode.h"
#include "hantekprotocol/controlcode.h"
#include "hantekprotocol/definitions.h"
#include <QList>

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsBulk                  hantek/control.h
/// \brief Stores the bulk command codes used for this device.
struct ControlSpecificationCommandsBulk {
    BulkCode setChannels = (BulkCode)-1;     ///< Command for setting used channels
    BulkCode setSamplerate = (BulkCode)-1;   ///< Command for samplerate settings
    BulkCode setGain = (BulkCode)-1;         ///< Command for gain settings (Usually in combination with
                              /// CONTROL_SETRELAYS)
    BulkCode setRecordLength = (BulkCode)-1; ///< Command for buffer settings
    BulkCode setTrigger = (BulkCode)-1;      ///< Command for trigger settings
    BulkCode setPretrigger = (BulkCode)-1;   ///< Command for pretrigger settings
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsControl               hantek/control.h
/// \brief Stores the control command codes used for this device.
struct ControlSpecificationCommandsControl {
    ControlCode setOffset = (ControlCode)-1; ///< Command for setting offset calibration data
    ControlCode setRelays = (ControlCode)-1; ///< Command for setting gain relays (Usually in
                           /// combination with BULK_SETGAIN)
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecificationCommandsValues                hantek/control.h
/// \brief Stores the control value codes used for this device.
struct ControlSpecificationCommandsValues {
    ControlValue offsetLimits = VALUE_OFFSETLIMITS;  ///< Code for channel offset limits
    ControlValue voltageLimits = (ControlValue)-1; ///< Code for voltage limits
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
    ControlSamplerateLimits single = {50e6, 50e6, 0, std::vector<unsigned>()}; ///< The limits for single channel mode
    ControlSamplerateLimits multi  = {100e6, 100e6, 0, std::vector<unsigned>()};  ///< The limits for multi channel mode
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSpecification                              hantek/control.h
/// \brief Stores the specifications of the currently connected device.
struct ControlSpecification {
    // Interface
    ControlSpecificationCommands command; ///< The commands for this device

    // Limits
    ControlSpecificationSamplerate samplerate; ///< The samplerate specifications
    std::vector<unsigned int> bufferDividers;        ///< Samplerate dividers for record lengths
    std::vector<double> gainSteps;                   ///< Available voltage steps in V/screenheight
    unsigned char sampleSize;                  ///< Number of bits per sample

    // Calibration
    /// The sample values at the top of the screen
    std::vector<unsigned short> voltageLimit[HANTEK_CHANNELS];
    /// The index of the selected gain on the hardware
    std::vector<unsigned char> gainIndex;
    std::vector<unsigned char> gainDiv;
    std::vector<double> sampleSteps; ///< Available samplerate steps in s
    std::vector<unsigned char> sampleDiv;

    /// Calibration data for the channel offsets
    OffsetsPerGainStep offsetLimit[HANTEK_CHANNELS];

    bool isSoftwareTriggerDevice = false;
    bool useControlNoBulk = false;
    bool supportsCaptureState = true;
    bool supportsOffset = true;
    bool supportsCouplingRelays = true;
};

}

