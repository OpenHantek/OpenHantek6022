// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "enums.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekprotocol/types.h"

namespace Hantek {
struct CalibrationValues;
}

namespace Dso {

struct ControlSamplerateLimits;

/// \brief Stores the target samplerate settings of the device.
struct ControlSettingsSamplerateTarget {
    double samplerate; ///< The target samplerate set via setSamplerate
    double duration;   ///< The target record time set via setRecordTime
    enum SamplerrateSet { Duration, Samplerrate } samplerateSet;
};

/// \brief Stores the current samplerate settings of the device.
struct ControlSettingsSamplerate {
    ControlSettingsSamplerateTarget target; ///< The target samplerate values
    const ControlSamplerateLimits *limits;  ///< The samplerate limits
    unsigned int downsampler = 1;           ///< The variable downsampling factor
    double current = 1e6;                   ///< The current samplerate
};

/// \brief Stores the current trigger settings of the device.
struct ControlSettingsTrigger {
    std::vector< double > level;                    ///< The trigger level for each channel in V
    double position = 0.0;                          ///< The current pretrigger position
    unsigned int point = 0;                         ///< The trigger position in Hantek coding
    Dso::TriggerMode mode = Dso::TriggerMode::AUTO; ///< The trigger mode
    Dso::Slope slope = Dso::Slope::Positive;        ///< The trigger slope
    int source = 0;                                 ///< The trigger source
    int smooth = 0;                                 ///< Don't trigger on glitches
};

/// \brief Stores the current amplification settings of the device.
struct ControlSettingsVoltage {
    double offset = 0.0;                        ///< The screen offset for each channel
    unsigned gain = 0;                          ///< The gain id
    bool used = false;                          ///< true, if the channel is used
    bool inverted = false;                      ///< true, if the channel is inverted
    double probeAttn = 1.0;                     ///< attenuation of probe
    Dso::Coupling coupling = Dso::Coupling::DC; ///< The coupling
};

/// \brief Stores the current settings of the device.
struct ControlSettings {
    ControlSettings( const ControlSamplerateLimits *limits, size_t channelCount );
    ~ControlSettings();
    ControlSettings( const ControlSettings & ) = delete;
    ControlSettings operator=( const ControlSettings & ) = delete;
    ControlSettingsSamplerate samplerate;          ///< The samplerate settings
    std::vector< ControlSettingsVoltage > voltage; ///< The amplification settings
    ControlSettingsTrigger trigger;                ///< The trigger settings
    RecordLengthID recordLengthId = 1;             ///< The id in the record length array
    unsigned channelCount = 0;                     ///< Number of activated channels
    Hantek::CalibrationValues *calibrationValues;  ///< Calibration data for the channel offsets & gains
    Hantek::CalibrationValues *correctionValues;   ///< Online correction data for the channel offsets
    Hantek::ControlGetCalibration cmdGetCalibration;
};
} // namespace Dso
