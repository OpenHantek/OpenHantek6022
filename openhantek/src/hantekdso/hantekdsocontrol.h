// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "errorcodes.h"
#include "bulkStructs.h"
#include "controlStructs.h"
#include "dsosamples.h"
#include "states.h"
#include "controlspecification.h"
#include "controlsettings.h"
#include "controlindexes.h"
#include "utils/dataarray.h"
#include "utils/printutils.h"

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
    unsigned getChannelCount();

    /// \brief Get available record lengths for this oscilloscope.
    /// \return The number of physical channels, empty list for continuous.
    const std::vector<unsigned> &getAvailableRecordLengths();

    /// \brief Get minimum samplerate for this oscilloscope.
    /// \return The minimum samplerate for the current configuration in S/s.
    double getMinSamplerate();

    /// \brief Get maximum samplerate for this oscilloscope.
    /// \return The maximum samplerate for the current configuration in S/s.
    double getMaxSamplerate();

    /// \brief Get a list of the names of the special trigger sources.
    const QStringList *getSpecialTriggerSources();

    /// Return the associated usb device.
    USBDevice *getDevice();

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

  private:
    bool isRollMode() const;
    bool isFastRate() const;
    int getRecordLength() const;

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
    std::vector<unsigned char> getSamples(unsigned &previousSampleCount) const;

    /// \brief Converts raw oscilloscope data to sample data
    void convertRawDataToSamples(const std::vector<unsigned char> &rawData);

    /// \brief Sets the size of the sample buffer without updating dependencies.
    /// \param index The record length index that should be set.
    /// \return The record length that has been set, 0 on error.
    unsigned updateRecordLength(unsigned size);

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

  public: // TODO redo command queues
    /// Pointers to bulk commands, ready to be transmitted
    DataArray<unsigned char> *command[Hantek::BULK_COUNT] = {0};
    /// true, when the command should be executed
    bool commandPending[Hantek::BULK_COUNT] = {false};
    ///< Pointers to control commands
    DataArray<unsigned char> *control[Hantek::CONTROLINDEX_COUNT] = {0};
    ///< Request codes for control commands
    unsigned char controlCode[Hantek::CONTROLINDEX_COUNT];
    ///< true, when the control command should be executed
    bool controlPending[Hantek::CONTROLINDEX_COUNT] = {false};
  private:
    // Communication with device
    USBDevice *device;     ///< The USB device for the oscilloscope
    bool sampling = false; ///< true, if the oscilloscope is taking samples

    QStringList specialTriggerSources = {tr("EXT"), tr("EXT/10")}; ///< Names of the special trigger sources

    // Device setup
    Hantek::ControlSpecification specification; ///< The specifications of the device
    Hantek::ControlSettings controlsettings;    ///< The current settings of the device

    // Results
    DSOsamples result;
    unsigned previousSampleCount = 0; ///< The expected total number of samples at
                                      /// the last check before sampling started

    // State of the communication thread
    int captureState = Hantek::CAPTURE_WAITING;
    Hantek::RollState rollState = Hantek::RollState::STARTSAMPLING;
    bool _samplingStarted = false;
    Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode)-1;
    int cycleCounter = 0;
    int startCycle = 0;
    int cycleTime = 0;

  public slots:
    void startSampling();
    void stopSampling();

    Dso::ErrorCode setRecordLength(unsigned size);
    Dso::ErrorCode setSamplerate(double samplerate = 0.0);
    Dso::ErrorCode setRecordTime(double duration = 0.0);

    Dso::ErrorCode setChannelUsed(unsigned channel, bool used);
    Dso::ErrorCode setCoupling(unsigned channel, Dso::Coupling coupling);
    Dso::ErrorCode setGain(unsigned channel, double gain);
    Dso::ErrorCode setOffset(unsigned channel, double offset);

    Dso::ErrorCode setTriggerMode(Dso::TriggerMode mode);
    Dso::ErrorCode setTriggerSource(bool special, unsigned id);
    Dso::ErrorCode setTriggerLevel(unsigned channel, double level);
    Dso::ErrorCode setTriggerSlope(Dso::Slope slope);
    Dso::ErrorCode setPretriggerPosition(double position);
    void forceTrigger();

  signals:
    void samplingStarted();                                  ///< The oscilloscope started sampling/waiting for trigger
    void samplingStopped();                                  ///< The oscilloscope stopped sampling/waiting for trigger
    void statusMessage(const QString &message, int timeout); ///< Status message about the oscilloscope
    void samplesAvailable();                                 ///< New sample data is available

    void availableRecordLengthsChanged(const std::vector<unsigned> &recordLengths); ///< The available record
                                                                                    /// lengths, empty list for

    void samplerateLimitsChanged(double minimum, double maximum); ///< The minimum or maximum samplerate has changed
    void recordLengthChanged(unsigned long duration);             ///< The record length has changed
    void recordTimeChanged(double duration);                      ///< The record time duration has changed
    void samplerateChanged(double samplerate);                    ///< The samplerate has changed
    void samplerateSet(int mode, QList<double> sampleSteps);      ///< The samplerate has changed

    void communicationError() const;
};
