#pragma once

#include "enums.h"
#include "hantekprotocol/types.h"

namespace Hantek {
struct OffsetsPerGainStep;
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
    ControlSamplerateLimits *limits;        ///< The samplerate limits
    unsigned int downsampler = 1;           ///< The variable downsampling factor
    double current = 1e8;                   ///< The current samplerate
};

/// \brief Stores the current trigger settings of the device.
struct ControlSettingsTrigger {
    std::vector<double> level;                                   ///< The trigger level for each channel in V
    double position = 0.0;                                       ///< The current pretrigger position
    unsigned int point = 0;                                      ///< The trigger position in Hantek coding
    Dso::TriggerMode mode = Dso::TriggerMode::HARDWARE_SOFTWARE; ///< The trigger mode
    Dso::Slope slope = Dso::Slope::Positive;                     ///< The trigger slope
    bool special = false;                                        ///< true, if the trigger source is special
    unsigned int source = 0;                                     ///< The trigger source
};

/// \brief Stores the current amplification settings of the device.
struct ControlSettingsVoltage {
    double offset = 0.0;     ///< The screen offset for each channel
    double offsetReal = 0.0; ///< The real offset for each channel (Due to quantization)
    unsigned gain = 0;       ///< The gain id
    bool used = false;       ///< true, if the channel is used
};

/// \brief Stores the current settings of the device.
struct ControlSettings {
    ControlSettings(ControlSamplerateLimits *limits, size_t channelCount);
    ~ControlSettings();
    ControlSettingsSamplerate samplerate;        ///< The samplerate settings
    std::vector<ControlSettingsVoltage> voltage; ///< The amplification settings
    ControlSettingsTrigger trigger;              ///< The trigger settings
    RecordLengthID recordLengthId = 1;           ///< The id in the record length array
    unsigned usedChannels = 0;                   ///< Number of activated channels
    unsigned swSampleMargin = 2000;              ///< Software trigger, sample margin
    Hantek::OffsetsPerGainStep *offsetLimit;     ///< Calibration data for the channel offsets
};
}
