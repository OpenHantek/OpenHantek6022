// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QPointF>
#include <QString>

#include "hantekdso/controlspecification.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"
#include "viewconstants.h"
#include <vector>


/// \brief Holds the cursor parameters
struct DsoSettingsScopeCursor {
    enum CursorShape { NONE, HORIZONTAL, VERTICAL, RECTANGULAR } shape = NONE;
    QPointF pos[ MARKER_COUNT ] = {{-1.0, -1.0}, {1.0, 1.0}}; ///< Position in div
};

/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
    Dso::GraphFormat format = Dso::GraphFormat::TY; ///< Graph drawing mode of the scope
    double frequencybase = 1e3;                     ///< Frequencybase in Hz/div
    DsoSettingsScopeCursor cursor;

    unsigned int recordLength = 0; ///< Sample count
    double timebase = 1e-3;        ///< Timebase in s/div
    double maxTimebase = 0.1;      ///< Allow very slow timebases 0.1 ... 1.0 s/div
    double samplerate = 1e6;       ///< The samplerate of the oscilloscope in S
    double calfreq = 1e3;          ///< The frequency of the calibration output
};

/// \brief Holds the settings for the trigger.
/// TODO Use ControlSettingsTrigger
struct DsoSettingsScopeTrigger {
    Dso::TriggerMode mode = Dso::TriggerMode::AUTO; ///< Automatic, normal or single trigger
    double offset = 0.5;                            ///< Horizontal position for pretrigger (middle of screen)
    Dso::Slope slope = Dso::Slope::Positive;        ///< Rising or falling edge causes trigger
    unsigned int source = 0;                        ///< Channel that is used as trigger source
    bool smooth = false;                            ///< Don't trigger on glitches
};

/// \brief Base for DsoSettingsScopeSpectrum and DsoSettingsScopeVoltage
struct DsoSettingsScopeChannel {
    QString name;      ///< Name of this channel
    bool used = false; ///< true if the channel is turned on
    DsoSettingsScopeCursor cursor;
};

/// \brief Holds the settings for the spectrum analysis.
struct DsoSettingsScopeSpectrum : public DsoSettingsScopeChannel {
    double offset = 0.0;     ///< Vertical offset in divs
    double magnitude = 20.0; ///< The vertical resolution in dB/div
};

/// \brief Holds the settings for the normal voltage graphs.
/// TODO Use ControlSettingsVoltage
struct DsoSettingsScopeVoltage : public DsoSettingsScopeChannel {
    double offset = 0.0;              ///< Vertical offset in divs
    double trigger = 0.0;             ///< Trigger level in V
    unsigned gainStepIndex = 6;       ///< The vertical resolution in V/div (default = 1.0)
    unsigned couplingOrMathIndex = 0; ///< Different index: coupling for real- and mode for math-channels
    bool inverted = false;            ///< true if the channel is inverted (mirrored on cross-axis)
    double probeAttn = 1.0;           ///< attenuation of probe
};

/// \brief Holds the settings for the oscilloscope.
struct DsoSettingsScope {
    std::vector< double > gainSteps = {2e-2, 5e-2, 1e-1, 2e-1, 5e-1, 1e0, 2e0, 5e0}; ///< The selectable voltage gain steps in V/div
    std::vector< DsoSettingsScopeSpectrum > spectrum;                                ///< Spectrum analysis settings
    std::vector< DsoSettingsScopeVoltage > voltage;                                  ///< Settings for the normal graphs
    DsoSettingsScopeHorizontal horizontal;                                           ///< Settings for the horizontal axis
    DsoSettingsScopeTrigger trigger;                                                 ///< Settings for the trigger

    bool histogram = false;

    double gain( unsigned channel ) const { return gainSteps[ voltage[ channel ].gainStepIndex ] * voltage[ channel ].probeAttn; }

    bool anyUsed( ChannelID channel ) { return voltage[ channel ].used | spectrum[ channel ].used; }

    Dso::Coupling coupling( ChannelID channel, const Dso::ControlSpecification *deviceSpecification ) const {
        return deviceSpecification->couplings[ voltage[ channel ].couplingOrMathIndex ];
    }
    // Channels, including math channels
    unsigned countChannels() const { return unsigned( voltage.size() ); }

    double getMarker( unsigned int marker ) const {
        double x = qBound( MARGIN_LEFT, marker < MARKER_COUNT ? horizontal.cursor.pos[ marker ].x() : 0.0, MARGIN_RIGHT );
        return x;
    }

    void setMarker( unsigned int marker, double value ) {
        if ( marker < MARKER_COUNT )
            horizontal.cursor.pos[ marker ].setX( value );
    }
};
