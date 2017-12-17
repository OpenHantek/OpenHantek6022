#pragma once

#include "definitions.h"
#include <QList>

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeHorizontal                                settings.h
/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
    Dso::GraphFormat format;     ///< Graph drawing mode of the scope
    double frequencybase;        ///< Frequencybase in Hz/div
    double marker[MARKER_COUNT]; ///< Marker positions in div
    bool marker_visible[MARKER_COUNT];
    double timebase;           ///< Timebase in s/div
    unsigned int recordLength; ///< Sample count
    double samplerate;         ///< The samplerate of the oscilloscope in S
    bool samplerateSet;        ///< The samplerate was set by the user, not the timebase
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeTrigger                                   settings.h
/// \brief Holds the settings for the trigger.
struct DsoSettingsScopeTrigger {
    bool filter;           ///< Not sure what this is good for...
    Dso::TriggerMode mode; ///< Automatic, normal or single trigger
    double position;       ///< Horizontal position for pretrigger
    Dso::Slope slope;      ///< Rising or falling edge causes trigger
    unsigned int source;   ///< Channel that is used as trigger source
    bool special;          ///< true if the trigger source is not a standard channel
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
    int misc;       ///< Different enums, coupling for real- and mode for math-channels
    QString name;   ///< Name of this channel
    double offset;  ///< Vertical offset in divs
    double trigger; ///< Trigger level in V
    bool used;      ///< true if this channel is enabled
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScope                                          settings.h
/// \brief Holds the settings for the oscilloscope.
struct DsoSettingsScope {
    DsoSettingsScopeHorizontal horizontal;    ///< Settings for the horizontal axis
    DsoSettingsScopeTrigger trigger;          ///< Settings for the trigger
    QList<DsoSettingsScopeSpectrum> spectrum; ///< Spectrum analysis settings
    QList<DsoSettingsScopeVoltage> voltage;   ///< Settings for the normal graphs

    unsigned int physicalChannels;      ///< Number of real channels (No math etc.)
    Dso::WindowFunction spectrumWindow; ///< Window function for DFT
    double spectrumReference;           ///< Reference level for spectrum in dBm
    double spectrumLimit;               ///< Minimum magnitude of the spectrum (Avoids peaks)
};
