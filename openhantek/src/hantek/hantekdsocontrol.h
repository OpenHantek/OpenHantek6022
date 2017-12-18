// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "bulkStructs.h"
#include "controlStructs.h"
#include "dsosamples.h"
#include "stateStructs.h"
#include "utils/dataarray.h"
#include "utils/printutils.h"

#include <vector>

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QTimer>

class USBDevice;

/// \brief The DsoControl abstraction layer for %Hantek USB DSOs.
class HantekDsoControl : public QObject {
    Q_OBJECT

  public:
    /**
     * Creates a dsoControl object. The actual event loop / timer is not started.
     * You can optionally create a thread and move the created object to the
     * thread.
     * You need to call updateInterval() to start the timer.
     * @param device
     */
    HantekDsoControl(USBDevice *device);
    ~HantekDsoControl();

    /// Call this to start the processing. This method will call itself
    /// periodically from there on.
    /// It is wise to move this class object to an own thread and call run from
    /// there.
    void run();

    unsigned getChannelCount();
    QList<unsigned> *getAvailableRecordLengths();
    double getMinSamplerate();
    double getMaxSamplerate();

    const QStringList *getSpecialTriggerSources();
    const USBDevice *getDevice() const;
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
    int stringCommand(const QString &commandString);
  signals:
    void samplingStarted();                                  ///< The oscilloscope started sampling/waiting for trigger
    void samplingStopped();                                  ///< The oscilloscope stopped sampling/waiting for trigger
    void statusMessage(const QString &message, int timeout); ///< Status message about the oscilloscope
    void samplesAvailable();                                 ///< New sample data is available

    void availableRecordLengthsChanged(const QList<unsigned> &recordLengths); ///< The available record
                                                                              /// lengths, empty list for
    /// continuous
    void samplerateLimitsChanged(double minimum, double maximum); ///< The minimum or maximum samplerate has changed
    void recordLengthChanged(unsigned long duration);             ///< The record length has changed
    void recordTimeChanged(double duration);                      ///< The record time duration has changed
    void samplerateChanged(double samplerate);                    ///< The samplerate has changed
    void samplerateSet(int mode, QList<double> sampleSteps);      ///< The samplerate has changed

    void communicationError();

  protected:
    bool isRollMode();
    int getRecordLength();
    void updateInterval();
    unsigned calculateTriggerPoint(unsigned value);
    int getCaptureState();
    int getSamples(bool process);
    double getBestSamplerate(double samplerate, bool fastRate = false, bool maximum = false, unsigned *downsampler = 0);
    unsigned getSampleCount(bool *fastRate = 0);
    unsigned updateRecordLength(unsigned size);
    unsigned updateSamplerate(unsigned downsampler, bool fastRate);
    void restoreTargets();
    void updateSamplerateLimits();

    // Communication with device
    USBDevice *device;     ///< The USB device for the oscilloscope
    bool sampling = false; ///< true, if the oscilloscope is taking samples

    QStringList specialTriggerSources = {tr("EXT"),tr("EXT/10")}; ///< Names of the special trigger sources

    DataArray<unsigned char> *command[Hantek::BULK_COUNT] = {0}; ///< Pointers to bulk
                                                           /// commands, ready to
    /// be transmitted
    bool commandPending[Hantek::BULK_COUNT] = {false};                       ///< true, when the command should be
                                                                   /// executed
    DataArray<unsigned char> *control[Hantek::CONTROLINDEX_COUNT] = {0}; ///< Pointers to control commands
    unsigned char controlCode[Hantek::CONTROLINDEX_COUNT];         ///< Request codes for
                                                                   /// control commands
    bool controlPending[Hantek::CONTROLINDEX_COUNT]= {false};               ///< true, when the control
    /// command should be executed

    // Device setup
    Hantek::ControlSpecification specification; ///< The specifications of the device
    Hantek::ControlSettings controlsettings;           ///< The current settings of the device

    // Results
    DSOsamples result;
    unsigned previousSampleCount = 0; ///< The expected total number of samples at
                                  /// the last check before sampling started

    // State of the communication thread
    int captureState = Hantek::CAPTURE_WAITING;
    int rollState = 0;
    bool _samplingStarted = false;
    Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode)-1;
    int cycleCounter = 0;
    int startCycle = 0;
    int cycleTime = 0;

  public slots:
    void startSampling();
    void stopSampling();

    unsigned setRecordLength(unsigned size);
    double setSamplerate(double samplerate = 0.0);
    double setRecordTime(double duration = 0.0);

    int setChannelUsed(unsigned channel, bool used);
    int setCoupling(unsigned channel, Dso::Coupling coupling);
    double setGain(unsigned channel, double gain);
    double setOffset(unsigned channel, double offset);

    int setTriggerMode(Dso::TriggerMode mode);
    int setTriggerSource(bool special, unsigned id);
    double setTriggerLevel(unsigned channel, double level);
    int setTriggerSlope(Dso::Slope slope);
    double setPretriggerPosition(double position);
    int forceTrigger();
};
