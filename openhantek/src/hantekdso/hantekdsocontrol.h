// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX 1 // disable windows.h min/max global methods
#endif
#endif

#include <limits>

#include "controlsettings.h"
#include "controlspecification.h"
#include "dsosamples.h"
#include "errorcodes.h"
#include "mathchannel.h"
#include "scopesettings.h"
#include "triggering.h"
#include "utils/printutils.h"
#include "viewconstants.h"

#include "hantekprotocol/controlStructs.h"
#include "hantekprotocol/definitions.h"

#include "dsomodel.h"

#include <vector>

#include <QSettings>
#include <QThread>

class Capturing;
class ScopeDevice;

struct Raw {
    unsigned channels = 0;
    double samplerate = 0;
    unsigned oversampling = 0;
    unsigned gainValue[ 2 ] = { 1, 1 }; // 1,2,5,10,..
    unsigned gainIndex[ 2 ] = { 7, 7 }; // index 0..7
    unsigned tag = 0;
    bool freeRun = false;  // small buffer, no trigger
    bool valid = false;    // samples can be processed
    bool rollMode = false; // one complete buffer received, start to roll
    unsigned size = 0;
    unsigned received = 0;
    std::vector< unsigned char > data;
    mutable QReadWriteLock lock;
};


/// \brief The DsoControl abstraction layer for %Hantek USB DSOs.
/// TODO Please anyone, refactor this class into smaller pieces (Separation of Concerns!).
class HantekDsoControl : public QObject {
    Q_OBJECT
    friend Capturing;

  public:
    /**
     * Creates a dsoControl object. The actual event loop / timer is not started.
     * You can optionally create a thread and move the created object to the
     * thread.
     * You need to call updateInterval() to start the timer. This is done implicitly
     * if run() is called.
     * @param device The usb device. This object does not take ownership.
     */
    explicit HantekDsoControl( ScopeDevice *scopeDevice, const DSOModel *model, int verboseLevel );

    /// \brief Cleans up
    ~HantekDsoControl() override;

    /// Call this to start the processing.
    /// This method will call itself periodically from there on.
    /// Move this class object to an own thread and call run from there.
    void stateMachine();

    void stopStateMachine() { stateMachineRunning = false; }

    double getSamplerate() const { return controlsettings.samplerate.current; }

    unsigned getSamplesize() const {
        if ( controlsettings.trigger.mode == Dso::TriggerMode::ROLL )
            return SAMPLESIZE_ROLL;
        else
            return SAMPLESIZE;
    }

    bool isSamplingUI() const { return samplingUI; }

    /// Return the associated usb device.
    const ScopeDevice *getDevice() const { return scopeDevice; }

    /// Return the associated scope model.
    const DSOModel *getModel() const { return model; }


    /// \brief Sends control commands directly.
    /// <p>
    ///		<b>Syntax:</b><br />
    ///		<br />
    ///		%Control command:
    ///		<pre>cc [<em>hex code</em>] [<em>hex data</em>]</pre>
    /// </p>
    /// \param command The command as string (Has to be parsed).
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode stringCommand( const QString &commandString );

    void addCommand( ControlCommand *newCommand, bool pending = true );

    template < class T > T *modifyCommand( Hantek::ControlCode code ) {
        control[ uint8_t( code ) ]->pending = true;
        return static_cast< T * >( control[ uint8_t( code ) ] );
    }

    bool hasCommand( Hantek::ControlCode code ) { return ( control[ uint8_t( code ) ] != nullptr ); }

    const ControlCommand *getCommand( Hantek::ControlCode code ) const { return control[ uint8_t( code ) ]; }

    /// \brief Stops the device.
    void quitSampling();

    /// \brief Saves calibration settings e.g. to the scope's EEPROM
    void prepareForShutdown();

  private:
    std::unique_ptr< MathChannel > mathChannel;
    std::unique_ptr< Triggering > triggering;
    bool singleChannel = false;
    int verboseLevel = 0;
    void setSingleChannel( bool single ) { singleChannel = single; }
    bool isSingleChannel() const { return singleChannel; }
    bool triggerModeNONE() { return controlsettings.trigger.mode == Dso::TriggerMode::ROLL; }
    unsigned getRecordLength() const;
    void setDownsampling( unsigned downsampling ) { downsamplingNumber = downsampling; }
    bool replaceCalibrationEEPROM = false;
    Dso::ErrorCode getCalibrationFromIniFile();
    Dso::ErrorCode getCalibrationFromEEPROM();
    Dso::ErrorCode updateCalibrationValues( bool useEEPROM = false );
    Dso::ErrorCode writeCalibrationToEEPROM();

    /// Get the number of samples that are expected returned by the scope.
    /// In rolling mode this depends on the usb speed and packet size.
    /// \return The total number of samples the scope should return.
    unsigned getSampleCount() const { return isSingleChannel() ? getRecordLength() : getRecordLength() * specification->channels; }

    /// adjust for skipping of minimal 2048 leading samples
    unsigned grossSampleCount( unsigned net ) const { return ( ( net + 1024 ) / 1024 + 2 ) * 1024; }

    /// calculate backwards to get multiples of 1000 (typical 20000 or 10000)
    unsigned netSampleCount( unsigned gross ) const { return ( ( gross - 1024 ) / 1000 - 1 ) * 1000; }

    void updateInterval();

    /// \brief Converts raw oscilloscope data to sample data
    void convertRawDataToSamples();

    /// \brief Restore the samplerate/timebase targets after divider updates.
    void restoreTargets();

    /// \brief Update the minimum and maximum supported samplerate.
    void updateSamplerateLimits();

    void controlSetSamplerate( uint8_t sampleIndex );

    /// Pointers to control commands
    ControlCommand *control[ 255 ] = { nullptr };
    ControlCommand *firstControlCommand = nullptr;

    // Communication with device
    ScopeDevice *scopeDevice;  ///< The USB device for the oscilloscope
    bool deviceNotConnected(); ///< USB status, always false for demo device
    bool samplingUI = false;   ///< true, if the oscilloscope is taking samples

    // Device setup
    const DSOModel *model;                          ///< The attached scope model
    const Dso::ControlSpecification *specification; ///< The specifications of the device
    Dso::ControlSettings controlsettings;           ///< The current settings of the device
    const DsoSettingsScope *scope = nullptr;        ///< Global scope parameters and configurations

    // Results
    unsigned downsamplingNumber = 1; ///< Number of downsamples to reduce sample rate
    DSOsamples result;
    unsigned expectedSampleCount = 0; ///< The expected total number of samples at
                                      /// the last check before sampling started
    bool calibrationHasChanged = false;
    std::unique_ptr< QSettings > calibrationSettings;
    double offsetCorrection[ HANTEK_GAIN_STEPS ][ HANTEK_CHANNEL_NUMBER ];
    double gainCorrection[ HANTEK_GAIN_STEPS ][ HANTEK_CHANNEL_NUMBER ];
    bool capturing = false;
    bool samplingStarted = false;
    bool stateMachineRunning = false;
    int acquireInterval = 0;
    int displayInterval = 0;
    unsigned activeChannels = 2;
    bool refresh = false; // parameter changed -> new raw to result conversion and trigger search needed
    void requestRefresh( bool active = true ) { refresh = active; }
    bool refreshNeeded() {
        bool changed = refresh;
        refresh = false;
        return changed;
    }
    Raw raw;
    unsigned debugLevel = 0;
    uint8_t channelOffset[ 2 ] = { 0x80, 0x80 };

#define dprintf( level, fmt, ... )               \
    do {                                         \
        if ( debugLevel & level )                \
            fprintf( stderr, fmt, __VA_ARGS__ ); \
    } while ( 0 )

  public slots:
    /// \brief If sampling is disabled, no samplesAvailable() signals are send anymore, no samples
    /// are fetched from the device and no processing takes place.
    /// \param enabled Enables/Disables sampling
    void enableSamplingUI( bool enabled = true );

    /// \brief Sets the samplerate of the oscilloscope.
    /// \param samplerate The samplerate that should be met (S/s), 0.0 to restore
    /// current samplerate.
    /// \return The samplerate that has been set, 0.0 on error.
    Dso::ErrorCode setSamplerate( double samplerate = 0.0 );

    /// \brief Sets the time duration of one acquisition by adapting the samplerate.
    /// \param duration The record time duration that should be met (s), 0.0 to
    /// restore current record time.
    /// \return The record time duration that has been set, 0.0 on error.
    Dso::ErrorCode setRecordTime( double duration = 0.0 );

    /// \brief Enables/disables filtering of the given channel.
    /// \param channel The channel that should be set.
    /// \param used true if the channel should be sampled.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setChannelUsed( ChannelID channel, bool used );

    /// \brief Enables/disables inverting of the given channel.
    /// \param channel The channel that should be set.
    /// \param used true if the channel is inverted.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setChannelInverted( ChannelID channel, bool inverted );

    /// \brief Sets the gain for the given channel.
    /// Get the actual gain by specification.gainSteps[gainId]
    /// \param channel The channel that should be set.
    /// \param gain The gain that should be met (V/div).
    /// \return The gain that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setProbe( ChannelID channel, double probeAttn );

    /// \brief Sets the probe gain for the given channel.
    /// \param channel The channel that should be set.
    /// \param probeAttn gain of probe is set.
    /// \return error code.
    Dso::ErrorCode setGain( ChannelID channel, double gain );

    /// \brief Sets the coupling for the given channel.
    /// \param channel The channel that should be set.
    /// \param coupling The coupling that should be set.
    /// \return error code.
    Dso::ErrorCode setCoupling( ChannelID channel, Dso::Coupling coupling );

    /// \brief Set the trigger mode.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerMode( Dso::TriggerMode mode );

    /// \brief Set the trigger source.
    /// \param id The channel that should be used as trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSource( int channel );

    /// \brief Set the trigger smoothing.
    /// \param smooth The filter value.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSmooth( int smooth );

    /// \brief Set the trigger level.
    /// \param channel The channel that should be set.
    /// \param level The new trigger level (V).
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerLevel( ChannelID channel, double level );

    /// \brief Set the trigger slope.
    /// \param slope The Slope that should cause a trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSlope( Dso::Slope slope );

    /// \brief Set the trigger position.
    /// \param position The new trigger position (in s).
    /// \return The trigger position that has been set.
    Dso::ErrorCode setTriggerPosition( double position );

    /// \brief Sets the calibration frequency of the oscilloscope.
    /// \param calfreq The calibration frequency.
    /// \return The tfrequency that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setCalFreq( double calfreq = 0.0 );

    /// \brief Initializes the device with the current settings.
    /// \param scope The settings for the oscilloscope.
    void applySettings( DsoSettingsScope *scope );

    /// \brief Starts a new sampling block.
    void restartSampling();

    /// \brief enable/disable offset calibration
    void calibrateOffset( bool enable );

  signals:
    void showSamplingStatus( bool enabled );                   ///< The oscilloscope started/stopped sampling/waiting for trigger
    void statusMessage( const QString &message, int timeout ); ///< Status message about the oscilloscope
    void samplesAvailable( const DSOsamples *samples );        ///< New sample data is available

    /// The available samplerate range has changed
    void samplerateLimitsChanged( double minimum, double maximum );
    /// The available samplerate for fixed samplerate devices has changed
    void samplerateSet( int mode, QList< double > sampleSteps );

    void samplerateChanged( double samplerate ); ///< The samplerate has changed

    void communicationError() const;

    void liveCalibrationError() const; // live calibration stopped due to noise or big offset
};

Q_DECLARE_METATYPE( DSOsamples * )
