// SPDX-License-Identifier: GPL-2.0+

#pragma once

#define NOMINMAX // disable windows.h min/max global methods
#include <limits>

#include "controlsettings.h"
#include "controlspecification.h"
#include "dsosamples.h"
#include "errorcodes.h"
#include "states.h"
#include "utils/printutils.h"

#include "hantekprotocol/bulkStructs.h"
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
    HantekDsoControl(USBDevice *device);

    /// \brief Cleans up
    ~HantekDsoControl();

    /// Call this to start the processing. This method will call itself
    /// periodically from there on.
    /// It is wise to move this class object to an own thread and call run from
    /// there.
    void run();

    /// \brief Gets the physical channel count for this oscilloscope.
    /// \return The number of physical channels.
    unsigned getChannelCount() const;

    /// Return the read-only device control settings. Use the set- Methods to change
    /// device settings.
    const Dso::ControlSettings *getDeviceSettings() const;

    /// \brief Get available record lengths for this oscilloscope.
    /// \return The number of physical channels, empty list for continuous.
    const std::vector<unsigned> &getAvailableRecordLengths() const;

    /// \brief Get minimum samplerate for this oscilloscope.
    /// \return The minimum samplerate for the current configuration in S/s.
    double getMinSamplerate() const;

    /// \brief Get maximum samplerate for this oscilloscope.
    /// \return The maximum samplerate for the current configuration in S/s.
    double getMaxSamplerate() const;

    bool isSampling() const;

    /// Return the associated usb device.
    const USBDevice *getDevice() const;

    /// \brief Gets the speed of the connection.
    /// \return The ::ConnectionSpeed of the USB connection.
    int getConnectionSpeed() const;

    /// \brief Gets the maximum size of one packet transmitted via bulk transfer.
    /// \return The maximum packet size in bytes, negative libusb error code on error.
    int getPacketSize() const;

    /// Return the last sample set
    const DSOsamples &getLastSamples();

    /// \brief Sends bulk/control commands directly.
    /// <p>
    ///		<b>Syntax:</b><br />
    ///		<br />
    ///		Bulk command:
    ///		<pre>send bulk [<em>hex data</em>]</pre>
    ///		%Control command:
    ///		<pre>send control [<em>hex code</em>] [<em>hex data</em>]</pre>
    /// </p>
    /// \param command The command as string (Has to be parsed).
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode stringCommand(const QString &commandString);

    void addCommand(BulkCommand *newCommand, bool pending = true);
    template <class T> T *modifyCommand(Hantek::BulkCode code) {
        command[(uint8_t)code]->pending = true;
        return static_cast<T *>(command[(uint8_t)code]);
    }
    const BulkCommand *getCommand(Hantek::BulkCode code) const;

    void addCommand(ControlCommand *newCommand, bool pending = true);
    template <class T> T *modifyCommand(Hantek::ControlCode code) {
        control[(uint8_t)code]->pending = true;
        return static_cast<T *>(control[(uint8_t)code]);
    }
    const ControlCommand *getCommand(Hantek::ControlCode code) const;

  private:
    bool isRollMode() const;
    bool isFastRate() const;
    unsigned getRecordLength() const;

    Dso::ErrorCode retrieveChannelLevelData();

    /// \brief Calculated the nearest samplerate supported by the oscilloscope.
    /// \param samplerate The target samplerate, that should be met as good as
    /// possible.
    /// \param fastRate true, if the fast rate mode is enabled.
    /// \param maximum The target samplerate is the maximum allowed when true, the
    /// minimum otherwise.
    /// \param downsampler Pointer to where the selected downsampling factor should
    /// be written.
    /// \return The nearest samplerate supported, 0.0 on error.
    double getBestSamplerate(double samplerate, bool fastRate = false, bool maximum = false,
                             unsigned *downsampler = 0) const;

    /// Get the number of samples that are expected returned by the scope.
    /// In rolling mode this is depends on the usb speed and packet size.
    /// \return The total number of samples the scope should return.
    unsigned getSampleCount() const;

    void updateInterval();

    /// \brief Calculates the trigger point from the CommandGetCaptureState data.
    /// \param value The data value that contains the trigger point.
    /// \return The calculated trigger point for the given data.
    static unsigned calculateTriggerPoint(unsigned value);

    /// \brief Gets the current state.
    /// \return The current CaptureState of the oscilloscope.
    std::pair<int, unsigned> getCaptureState() const;

    /// \brief Gets sample data from the oscilloscope
    std::vector<unsigned char> getSamples(unsigned &expectedSampleCount) const;

    /// \brief Converts raw oscilloscope data to sample data
    void convertRawDataToSamples(const std::vector<unsigned char> &rawData);

    /// \brief Sets the size of the sample buffer without updating dependencies.
    /// \param index The record length index that should be set.
    /// \return The record length that has been set, 0 on error.
    unsigned updateRecordLength(RecordLengthID size);

    /// \brief Sets the samplerate based on the parameters calculated by
    /// Control::getBestSamplerate.
    /// \param downsampler The downsampling factor.
    /// \param fastRate true, if one channel uses all buffers.
    /// \return The downsampling factor that has been set.
    unsigned updateSamplerate(unsigned downsampler, bool fastRate);

    /// \brief Restore the samplerate/timebase targets after divider updates.
    void restoreTargets();

    /// \brief Update the minimum and maximum supported samplerate.
    void updateSamplerateLimits();

  private:
    /// Pointers to bulk/control commands
    BulkCommand *command[255] = {0};
    BulkCommand *firstBulkCommand = nullptr;
    ControlCommand *control[255] = {0};
    ControlCommand *firstControlCommand = nullptr;

    // Communication with device
    USBDevice *device;     ///< The USB device for the oscilloscope
    bool sampling = false; ///< true, if the oscilloscope is taking samples

    // Device setup
    const Dso::ControlSpecification *specification; ///< The specifications of the device
    Dso::ControlSettings controlsettings;           ///< The current settings of the device

    // Results
    DSOsamples result;
    unsigned expectedSampleCount = 0; ///< The expected total number of samples at
                                      /// the last check before sampling started

    // State of the communication thread
    int captureState = Hantek::CAPTURE_WAITING;
    Hantek::RollState rollState = Hantek::RollState::STARTSAMPLING;
    bool _samplingStarted = false;
    Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode)-1;
    int cycleCounter = 0;
    int startCycle = 0;
    int cycleTime = 0;

    /// \brief Send a bulk command to the oscilloscope.
    /// \param command The command, that should be sent.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    int bulkCommand(const std::vector<unsigned char> *command, int attempts = HANTEK_ATTEMPTS) const;

  public slots:
    /// \brief If sampling is disabled, no samplesAvailable() signals are send anymore, no samples
    /// are fetched from the device and no processing takes place.
    /// \param enabled Enables/Disables sampling
    void enableSampling(bool enabled);

    /// \brief Sets the size of the oscilloscopes sample buffer.
    /// \param index The record length index that should be set.
    /// \return The record length that has been set, 0 on error.
    Dso::ErrorCode setRecordLength(unsigned size);
    /// \brief Sets the samplerate of the oscilloscope.
    /// \param samplerate The samplerate that should be met (S/s), 0.0 to restore
    /// current samplerate.
    /// \return The samplerate that has been set, 0.0 on error.
    Dso::ErrorCode setSamplerate(double samplerate = 0.0);
    /// \brief Sets the time duration of one aquisition by adapting the samplerate.
    /// \param duration The record time duration that should be met (s), 0.0 to
    /// restore current record time.
    /// \return The record time duration that has been set, 0.0 on error.
    Dso::ErrorCode setRecordTime(double duration = 0.0);

    /// \brief Enables/disables filtering of the given channel.
    /// \param channel The channel that should be set.
    /// \param used true if the channel should be sampled.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setChannelUsed(ChannelID channel, bool used);
    /// \brief Set the coupling for the given channel.
    /// \param channel The channel that should be set.
    /// \param coupling The new coupling for the channel.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setCoupling(ChannelID channel, Dso::Coupling coupling);
    /// \brief Sets the gain for the given channel.
    /// Get the actual gain by specification.gainSteps[gainId]
    /// \param channel The channel that should be set.
    /// \param gain The gain that should be met (V/div).
    /// \return The gain that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setGain(ChannelID channel, double gain);
    /// \brief Set the offset for the given channel.
    /// Get the actual offset for the channel from controlsettings.voltage[channel].offsetReal
    /// \param channel The channel that should be set.
    /// \param offset The new offset value (0.0 - 1.0).
    Dso::ErrorCode setOffset(ChannelID channel, const double offset);

    /// \brief Set the trigger mode.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerMode(Dso::TriggerMode mode);
    /// \brief Set the trigger source.
    /// \param special true for a special channel (EXT, ...) as trigger source.
    /// \param id The number of the channel, that should be used as trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSource(bool special, unsigned id);
    /// \brief Set the trigger level.
    /// \param channel The channel that should be set.
    /// \param level The new trigger level (V).
    /// \return The trigger level that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setTriggerLevel(ChannelID channel, double level);
    /// \brief Set the trigger slope.
    /// \param slope The Slope that should cause a trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSlope(Dso::Slope slope);
    /// \brief Set the trigger position.
    /// \param position The new trigger position (in s).
    /// \return The trigger position that has been set.
    Dso::ErrorCode setPretriggerPosition(double position);
    void forceTrigger();

  signals:
    void samplingStatusChanged(bool enabled); ///< The oscilloscope started/stopped sampling/waiting for trigger
    void statusMessage(const QString &message, int timeout); ///< Status message about the oscilloscope
    void samplesAvailable(const DSOsamples *samples);        ///< New sample data is available

    void availableRecordLengthsChanged(const std::vector<unsigned> &recordLengths); ///< The available record
                                                                                    /// lengths, empty list for

    /// The available samplerate range has changed
    void samplerateLimitsChanged(double minimum, double maximum);
    /// The available samplerate for fixed samplerate devices has changed
    void samplerateSet(int mode, QList<double> sampleSteps);

    void recordLengthChanged(unsigned long duration); ///< The record length has changed
    void recordTimeChanged(double duration);          ///< The record time duration has changed
    void samplerateChanged(double samplerate);        ///< The samplerate has changed

    void communicationError() const;
};

Q_DECLARE_METATYPE(DSOsamples *)
