#pragma once

#include "definitions.h"
#include <QVector>
#include "analyse/enums.h"
#include "hantekdso/enums.h"

#define MARKER_COUNT 2 ///< Number of markers

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeHorizontal                                settings.h
/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
    Dso::GraphFormat format = Dso::GRAPHFORMAT_TY; ///< Graph drawing mode of the scope
    double frequencybase = 1e3;                    ///< Frequencybase in Hz/div
    double marker[MARKER_COUNT] = {-1.0, 1.0};     ///< Marker positions in div
    bool marker_visible[MARKER_COUNT];
    double timebase = 1e-3;        ///< Timebase in s/div
    unsigned int recordLength = 0; ///< Sample count
    double samplerate = 1e6;       ///< The samplerate of the oscilloscope in S
    bool samplerateSet = false;    ///< The samplerate was set by the user, not the timebase
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeTrigger                                   settings.h
/// \brief Holds the settings for the trigger.
struct DsoSettingsScopeTrigger {
    bool filter = true;                              ///< Not sure what this is good for...
    Dso::TriggerMode mode = Dso::TRIGGERMODE_NORMAL; ///< Automatic, normal or single trigger
    double position = 0.0;                           ///< Horizontal position for pretrigger
    Dso::Slope slope = Dso::SLOPE_POSITIVE;          ///< Rising or falling edge causes trigger
    unsigned int source = 0;                         ///< Channel that is used as trigger source
    bool special = false;                            ///< true if the trigger source is not a standard channel
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeSpectrum                                  settings.h
/// \brief Holds the settings for the spectrum analysis.
struct DsoSettingsScopeSpectrum {
    double magnitude; ///< The vertical resolution in dB/div
    QString name;     ///< Name of this channel
    double offset;    ///< Vertical offset in divs
    bool used;        ///< true if the spectrum is turned on
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeVoltage                                   settings.h
/// \brief Holds the settings for the normal voltage graphs.
struct DsoSettingsScopeVoltage {
    double gain;    ///< The vertical resolution in V/div
    bool inverted;  ///< true if the channel is inverted (mirrored on cross-axis)
    union { ///< Different enums, coupling for real- and mode for math-channels
        Dso::MathMode math;
        Dso::Coupling coupling;
        int rawValue;
    };
    QString name;   ///< Name of this channel
    double offset;  ///< Vertical offset in divs
    double trigger; ///< Trigger level in V
    bool used;      ///< true if this channel is enabled
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScope                                          settings.h
/// \brief Holds the settings for the oscilloscope.
struct DsoSettingsScope {
    DsoSettingsScopeHorizontal horizontal;      ///< Settings for the horizontal axis
    DsoSettingsScopeTrigger trigger;            ///< Settings for the trigger
    QVector<DsoSettingsScopeSpectrum> spectrum; ///< Spectrum analysis settings
    QVector<DsoSettingsScopeVoltage> voltage;   ///< Settings for the normal graphs

    unsigned int physicalChannels = 0;                     ///< Number of real channels (No math etc.)
    Dso::WindowFunction spectrumWindow = Dso::WindowFunction::HANN; ///< Window function for DFT
    double spectrumReference = 0.0;                        ///< Reference level for spectrum in dBm
    double spectrumLimit = -20.0;                          ///< Minimum magnitude of the spectrum (Avoids peaks)
};
