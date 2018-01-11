// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QString>

#include "hantekdso/controlspecification.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"
#include "post/enums.h"
#include <vector>

#define MARKER_COUNT 2 ///< Number of markers

/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
    Dso::GraphFormat format = Dso::GraphFormat::TY; ///< Graph drawing mode of the scope
    double frequencybase = 1e3;                     ///< Frequencybase in Hz/div
    double marker[MARKER_COUNT] = {-1.0, 1.0};      ///< Marker positions in div
    bool marker_visible[MARKER_COUNT] = {true, true};

    unsigned int recordLength = 0; ///< Sample count

    /// TODO Use ControlSettingsSamplerateTarget
    double timebase = 1e-3;  ///< Timebase in s/div
    double samplerate = 1e6; ///< The samplerate of the oscilloscope in S
    enum SamplerateSource { Samplerrate, Duration } samplerateSource = Samplerrate;
};

/// \brief Holds the settings for the trigger.
/// TODO Use ControlSettingsTrigger
struct DsoSettingsScopeTrigger {
    Dso::TriggerMode mode = Dso::TriggerMode::HARDWARE_SOFTWARE; ///< Automatic, normal or single trigger
    double position = 0.0;                                       ///< Horizontal position for pretrigger
    Dso::Slope slope = Dso::Slope::Positive;                     ///< Rising or falling edge causes trigger
    unsigned int source = 0;                                     ///< Channel that is used as trigger source
    bool special = false;             ///< true if the trigger source is not a standard channel
    unsigned swTriggerThreshold = 7;  ///< Software trigger, threshold
    unsigned swTriggerSampleSet = 11; ///< Software trigger, sample set
};

/// \brief Holds the settings for the spectrum analysis.
struct DsoSettingsScopeSpectrum {
    ChannelID channel;
    double magnitude = 20.0; ///< The vertical resolution in dB/div
    QString name;            ///< Name of this channel
    double offset = 0.0;     ///< Vertical offset in divs
    bool used = false;       ///< true if the spectrum is turned on
};

/// \brief Holds the settings for the normal voltage graphs.
/// TODO Use ControlSettingsVoltage
struct DsoSettingsScopeVoltage {
    double offset = 0.0;        ///< Vertical offset in divs
    double trigger = 0.0;       ///< Trigger level in V
    unsigned gainStepIndex = 6; ///< The vertical resolution in V/div (default = 1.0)
    union {                     ///< Different enums, coupling for real- and mode for math-channels
        Dso::MathMode math;
        unsigned couplingIndex = 0;
        int rawValue;
    };
    QString name;          ///< Name of this channel
    bool inverted = false; ///< true if the channel is inverted (mirrored on cross-axis)
    bool used = false;     ///< true if this channel is enabled
};

/// \brief Holds the settings for the oscilloscope.
struct DsoSettingsScope {
    std::vector<double> gainSteps = {1e-2, 2e-2, 5e-2, 1e-1, 2e-1,
                                     5e-1, 1e0,  2e0,  5e0};        ///< The selectable voltage gain steps in V/div
    Dso::WindowFunction spectrumWindow = Dso::WindowFunction::HANN; ///< Window function for DFT
    std::vector<DsoSettingsScopeSpectrum> spectrum;                 ///< Spectrum analysis settings
    std::vector<DsoSettingsScopeVoltage> voltage;                   ///< Settings for the normal graphs
    DsoSettingsScopeHorizontal horizontal;                          ///< Settings for the horizontal axis
    DsoSettingsScopeTrigger trigger;                                ///< Settings for the trigger
    double spectrumReference = 0.0;                                 ///< Reference level for spectrum in dBm
    double spectrumLimit = -20.0; ///< Minimum magnitude of the spectrum (Avoids peaks)

    double gain(unsigned channel) const { return gainSteps[voltage[channel].gainStepIndex]; }
    bool anyUsed(ChannelID channel) { return voltage[channel].used | spectrum[channel].used; }

    Dso::Coupling coupling(ChannelID channel, const Dso::ControlSpecification *deviceSpecification) {
        return deviceSpecification->couplings[voltage[channel].couplingIndex];
    }
    // Channels, including math channels
    unsigned countChannels() { return (unsigned)voltage.size(); }
};
