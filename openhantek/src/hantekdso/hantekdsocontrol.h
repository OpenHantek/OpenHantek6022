// SPDX-License-Identifier: GPL-2.0+

#pragma once

#define NOMINMAX // disable windows.h min/max global methods
#include <limits>

#include "controlsettings.h"
#include "controlspecification.h"
#include "dsosamples.h"
#include "errorcodes.h"
#include "scopesettings.h"
#include "states.h"
#include "utils/printutils.h"
#include "viewconstants.h"

#include "hantekprotocol/controlStructs.h"
#include "hantekprotocol/definitions.h"

#include <vector>

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QTimer>

class USBDevice;

/// \brief The DsoControl abstraction layer for %Hantek USB DSOs.
/// TODO Please anyone, refactor this class into smaller pieces (Separation of Concerns!).
class HantekDsoControl : public QObject {
    Q_OBJECT

  public:
    /**
     * Creates a dsoControl object. The actual event loop / timer is not started.
     * You can optionally create a thread and move the created object to the
     * thread.
     * You need to call updateInterval() to start the timer. This is done implicitly
     * if run() is called.
     * @param device The usb device. This object does not take ownership.
     */
    explicit HantekDsoControl( USBDevice *device );

    /// \brief Cleans up
    ~HantekDsoControl();

    /// Call this to start the processing.
    /// This method will call itself periodically from there on.
    /// Move this class object to an own thread and call run from there.
    void run();

    double getSamplerate() const { return controlsettings.samplerate.current; }

    unsigned getSamplesize() const { return SAMPLESIZE_USED; }

    bool isSampling() const { return sampling; }

    /// Return the associated usb device.
    const USBDevice *getDevice() const { return device; }


    /// \brief Sends control commands directly.
    /// <p>
    ///		<b>Syntax:</b><br />
    ///		<br />
    ///		%Control command:
    ///		<pre>send control [<em>hex code</em>] [<em>hex data</em>]</pre>
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
    void stopSampling();

// Attic for no more used public procedures
#if 0
    /// \brief Gets the physical channel count for this oscilloscope.
    /// \return The number of physical channels.
    unsigned getChannelCount() const { return specification->channels; }

    /// Return the read-only device control settings. Use the set- Methods to change
    /// device settings.
    const Dso::ControlSettings *getDeviceSettings() const { return &controlsettings; }

    /// \brief Get available record lengths for this oscilloscope.
    /// \return The number of physical channels, empty list for continuous.
    const std::vector<unsigned> &getAvailableRecordLengths() const {
        return controlsettings.samplerate.limits->recordLengths;
    }

    /// \brief Get minimum samplerate for this oscilloscope.
    /// \return The minimum samplerate for the current configuration in S/s.
    double getMinSamplerate() const {
        //printf( "getMinSamplerate\n" );
        return (double)specification->samplerate.single.base / specification->samplerate.single.maxDownsampler;
    }

    /// \brief Get maximum samplerate for this oscilloscope.
    /// \return The maximum samplerate for the current configuration in S/s.
    double getMaxSamplerate() const {
        //printf( "channelCount %d\n", controlsettings.channelCount );
        if (controlsettings.channelCount <= 1) {
            return specification->samplerate.multi.max;
        } else {
            return specification->samplerate.single.max;
        }
    }

    /// \brief Gets the speed of the connection.
    /// \return The ::ConnectionSpeed of the USB connection.
    int getConnectionSpeed() const {
        ControlGetSpeed response;
        int errorCode = device->controlRead(&response);
        if (errorCode < 0) return errorCode;
        return response.getSpeed();
    }

    /// \brief Gets the maximum size of one packet transmitted via bulk transfer.
    /// \return The maximum packet size in bytes, negative libusb error code on error.
    int getPacketSize() const {
        const int s = getConnectionSpeed();
        if (s == CONNECTION_FULLSPEED)
            return 64;
        else if (s == CONNECTION_HIGHSPEED)
            return 512;
        else if (s > CONNECTION_HIGHSPEED) {
            qWarning() << "Unknown USB speed. Please correct source code in USBDevice::getPacketSize()";
            throw new std::runtime_error("Unknown USB speed");
        } else if (s < 0)
            return s;
        return 0;
    }

    /// Return the last sample set
    const DSOsamples &getLastSamples() { return result; }
#endif

  private:
    bool fastRate = true;

    void setFastRate( bool fast ) { fastRate = fast; }

    bool isFastRate() const { return fastRate; }

    unsigned getRecordLength() const;

    void setDownsampling( unsigned downsampling ) { downsamplingNumber = downsampling; }

    Dso::ErrorCode retrieveChannelLevelData();

    /// Get the number of samples that are expected returned by the scope.
    /// In rolling mode this is depends on the usb speed and packet size.
    /// \return The total number of samples the scope should return.
    unsigned getSampleCount() const { return isFastRate() ? getRecordLength() : getRecordLength() * specification->channels; }

    void updateInterval();

    /// \brief Calculates the trigger point from the CommandGetCaptureState data.
    /// \param value The data value that contains the trigger point.
    /// \return The calculated trigger point for the given data.
    static unsigned calculateTriggerPoint( unsigned value );

    /// \brief Gets sample data from the oscilloscope
    std::vector< unsigned char > getSamples( unsigned &expectedSampleCount ) const;

    /// \brief Converts raw oscilloscope data to sample data
    void convertRawDataToSamples( const std::vector< unsigned char > &rawData, unsigned numChannels );

    /// \brief Sets the samplerate based on the parameters calculated by
    /// Control::getBestSamplerate.
    /// \param downsampler The downsampling factor.
    /// \param fastRate true, if one channel uses all buffers.
    /// \return The downsampling factor that has been set.
    unsigned updateSamplerate( unsigned downsampler );

    /// \brief Restore the samplerate/timebase targets after divider updates.
    void restoreTargets();

    /// \brief Update the minimum and maximum supported samplerate.
    void updateSamplerateLimits();

    unsigned searchTriggerPoint( Dso::Slope dsoSlope, unsigned int startPos = 0 );

    Dso::Slope mirrorSlope( Dso::Slope slope ) {
        return ( slope == Dso::Slope::Positive ? Dso::Slope::Negative : Dso::Slope::Positive );
    }

    unsigned softwareTrigger();

    bool triggering();

    /// Pointers to control commands
    ControlCommand *control[ 255 ] = {nullptr};
    ControlCommand *firstControlCommand = nullptr;

    // Communication with device
    USBDevice *device;     ///< The USB device for the oscilloscope
    bool sampling = false; ///< true, if the oscilloscope is taking samples

    // Device setup
    const Dso::ControlSpecification *specification; ///< The specifications of the device
    Dso::ControlSettings controlsettings;           ///< The current settings of the device

    // Results
    unsigned downsamplingNumber = 1; ///< Number of downsamples to reduce sample rate
    DSOsamples result;
    unsigned expectedSampleCount = 0; ///< The expected total number of samples at
                                      /// the last check before sampling started
    bool samplingStarted = false;
    int acquireInterval = 0;
    int displayInterval = 0;
    bool channelSetupChanged = false;
    unsigned triggeredPositionRaw = 0; // not triggered

  public slots:
    /// \brief If sampling is disabled, no samplesAvailable() signals are send anymore, no samples
    /// are fetched from the device and no processing takes place.
    /// \param enabled Enables/Disables sampling
    void enableSampling( bool enabled );

    /// \brief Sets the samplerate of the oscilloscope.
    /// \param samplerate The samplerate that should be met (S/s), 0.0 to restore
    /// current samplerate.
    /// \return The samplerate that has been set, 0.0 on error.
    Dso::ErrorCode setSamplerate( double samplerate = 0.0 );

    /// \brief Sets the time duration of one aquisition by adapting the samplerate.
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
    Dso::ErrorCode setTriggerSource( ChannelID channel, bool smooth );

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
    Dso::ErrorCode setTriggerOffset( double position );

    /// \brief Sets the calibration frequency of the oscilloscope.
    /// \param calfreq The calibration frequency.
    /// \return The tfrequency that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setCalFreq( double calfreq = 0.0 );

    /// \brief Initializes the device with the current settings.
    /// \param scope The settings for the oscilloscope.
    void applySettings( DsoSettingsScope *scope );

  signals:
    void samplingStatusChanged( bool enabled );                ///< The oscilloscope started/stopped sampling/waiting for trigger
    void statusMessage( const QString &message, int timeout ); ///< Status message about the oscilloscope
    void samplesAvailable( const DSOsamples *samples );        ///< New sample data is available

    /// The available samplerate range has changed
    void samplerateLimitsChanged( double minimum, double maximum );
    /// The available samplerate for fixed samplerate devices has changed
    void samplerateSet( int mode, QList< double > sampleSteps );

    void recordTimeChanged( double duration );   ///< The record time duration has changed
    void samplerateChanged( double samplerate ); ///< The samplerate has changed

    void communicationError() const;
};

Q_DECLARE_METATYPE( DSOsamples * )
