// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCoreApplication>
#include <QPointF>

#include "hantekdso/controlspecification.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"
#include "viewconstants.h"
#include <vector>


/// \brief Holds the cursor parameters
struct DsoSettingsScopeCursor {
    enum CursorShape { NONE, HORIZONTAL, VERTICAL, RECTANGULAR } shape = NONE;
    QPointF pos[ 2 ] = { { -1.0, -1.0 }, { 1.0, 1.0 } }; ///< Position in div
};

/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
    Dso::GraphFormat format = Dso::GraphFormat::TY; ///< Graph drawing mode of the scope
    double frequencybase = 1e3;                     ///< Frequencybase in Hz/div
    DsoSettingsScopeCursor cursor;

    int recordLength = 0;   ///< Sample count
    double timebase = 1e-3; ///< Timebase in s/div
    double maxTimebase = 1; ///< Allow very slow timebases 0.1 ... 10.0 s/div
#ifdef Q_PROCESSOR_ARM
    // RPi: Not more often than every 10 ms
    double acquireInterval = 0.010; ///< Minimal time between captured frames
#else
    // other PC: Not more often than every 1 ms
    double acquireInterval = 0.001; ///< Minimal time between captured frames
#endif
    double samplerate = 1e6; ///< The samplerate of the oscilloscope in S
    int dotsOnScreen = 0;
    double calfreq = 1e3; ///< The frequency of the calibration output
};

/// \brief Holds the settings for the trigger.
/// TODO Use ControlSettingsTrigger
struct DsoSettingsScopeTrigger {
    Dso::TriggerMode mode = Dso::TriggerMode::AUTO; ///< Automatic, normal or single trigger
    double position = 0.5;                          ///< Horizontal position for pretrigger (middle of screen)
    Dso::Slope slope = Dso::Slope::Positive;        ///< Rising or falling edge causes trigger
    int source = 0;                                 ///< Channel that is used as trigger source
    int smooth = 0;                                 ///< Don't trigger on glitches
};

/// \brief Base for DsoSettingsScopeSpectrum and DsoSettingsScopeVoltage
struct DsoSettingsScopeChannel {
    QString name;         ///< Name of this channel
    bool used = false;    ///< true if the channel is used (either visible or input for math etc.)
    bool visible = false; ///< true if the channel is turned on
    DsoSettingsScopeCursor cursor;
};

/// \brief Holds the settings for the spectrum analysis.
struct DsoSettingsScopeSpectrum : public DsoSettingsScopeChannel {
    double offset = 0.0;     ///< Vertical offset in divs
    double magnitude = 20.0; ///< The vertical resolution in dB/div
};

/// \brief Holds the settings for the power and frequency analysis.
struct DsoSettingsScopeAnalysis {
    double spectrumReference = 0.0; ///< Reference level for spectrum in dBV
    bool calculateDummyLoad = false;
    unsigned dummyLoad = 50; ///< Dummy load in  Ohms
    QString dBsuffixStrings[ 3 ] = { QCoreApplication::translate( "DsoSettingsScopeAnalysis", "V" ),
                                     QCoreApplication::translate( "DsoSettingsScopeAnalysis", "u" ),
                                     QCoreApplication::translate( "DsoSettingsScopeAnalysis", "m" ) };
    int dBsuffixIndex = 0; // dBV is default
    QString dBsuffix() {   // use current index
        return dBsuffixStrings[ dBsuffixIndex ];
    };
    QString dBsuffix( int index ) {
        if ( index >= 0 && index < 3 )       // valid suffix index
            return dBsuffixStrings[ index ]; // show this value
        else
            return QString();
    };
    bool calculateTHD = false;
    bool showNoteValue = false;
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
    std::vector< double > gainSteps = { 2e-2, 5e-2, 1e-1, 2e-1,
                                        5e-1, 1e0,  2e0,  5e0 }; ///< The selectable voltage gain steps in V/div
    std::vector< DsoSettingsScopeSpectrum > spectrum;            ///< Spectrum analysis settings
    std::vector< DsoSettingsScopeVoltage > voltage;              ///< Settings for the normal graphs
    DsoSettingsScopeHorizontal horizontal;                       ///< Settings for the horizontal axis
    DsoSettingsScopeTrigger trigger;                             ///< Settings for the trigger
    DsoSettingsScopeAnalysis analysis;                           ///< Settings for the analysis

    int verboseLevel = 0;
    int toolTipVisible = 1; // show hints for beginners, can be disabled in settings dialog
    bool histogram = false;
    bool hasACcoupling = false;
    bool hasACmodification = false;
    bool liveCalibrationActive = false;

    double gain( unsigned channel ) const { return gainSteps[ voltage[ channel ].gainStepIndex ] * voltage[ channel ].probeAttn; }

    bool anyUsed( ChannelID channel ) const { return voltage[ channel ].used || spectrum[ channel ].used; }

    Dso::Coupling coupling( ChannelID channel, const Dso::ControlSpecification *deviceSpecification ) const {
        return deviceSpecification->couplings[ voltage[ channel ].couplingOrMathIndex ];
    }
    // Channels, including math channels
    ChannelID countChannels() const { return ChannelID( voltage.size() ); }

    double getMarker( int marker ) const {
        double x = qBound( MARGIN_LEFT, marker < 2 ? horizontal.cursor.pos[ marker ].x() : 0.0, MARGIN_RIGHT );
        return x;
    }

    void setMarker( unsigned int marker, double value ) {
        if ( marker < 2 )
            horizontal.cursor.pos[ marker ].setX( value );
    }
};
