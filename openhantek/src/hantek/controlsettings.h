#pragma once

#include "definitions.h"

namespace Hantek {

class ControlSamplerateLimits;

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSettingsSamplerateTarget                   hantek/control.h
/// \brief Stores the target samplerate settings of the device.
struct ControlSettingsSamplerateTarget {
    double samplerate;  ///< The target samplerate set via setSamplerate
    double duration;    ///< The target record time set via setRecordTime
    bool samplerateSet; ///< true means samplerate was set last, false duration
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSettingsSamplerate                         hantek/control.h
/// \brief Stores the current samplerate settings of the device.
struct ControlSettingsSamplerate {
    ControlSettingsSamplerateTarget target; ///< The target samplerate values
    ControlSamplerateLimits *limits;        ///< The samplerate limits
    unsigned int downsampler;               ///< The variable downsampling factor
    double current;                         ///< The current samplerate
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSettingsTrigger                            hantek/control.h
/// \brief Stores the current trigger settings of the device.
struct ControlSettingsTrigger {
    double level[HANTEK_CHANNELS]; ///< The trigger level for each channel in V
    double position;               ///< The current pretrigger position
    unsigned int point;            ///< The trigger position in Hantek coding
    Dso::TriggerMode mode;         ///< The trigger mode
    Dso::Slope slope;              ///< The trigger slope
    bool special;                  ///< true, if the trigger source is special
    unsigned int source;           ///< The trigger source
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSettingsVoltage                            hantek/control.h
/// \brief Stores the current amplification settings of the device.
struct ControlSettingsVoltage {
    double offset;     ///< The screen offset for each channel
    double offsetReal; ///< The real offset for each channel (Due to quantization)
    unsigned gain;     ///< The gain id
    bool used;         ///< true, if the channel is used
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ControlSettings                                   hantek/control.h
/// \brief Stores the current settings of the device.
struct ControlSettings {
    ControlSettingsSamplerate samplerate;            ///< The samplerate settings
    ControlSettingsVoltage voltage[HANTEK_CHANNELS]; ///< The amplification settings
    ControlSettingsTrigger trigger;                  ///< The trigger settings
    unsigned int recordLengthId;                     ///< The id in the record length array
    unsigned short int usedChannels;                 ///< Number of activated channels
};

}

