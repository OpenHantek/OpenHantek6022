// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "utils/dataarray.h"
#include "controlStructs.h"
#include "bulkStructs.h"
#include "stateStructs.h"
#include "utils/printutils.h"

#include <vector>

#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QTimer>

class USBDevice;

//////////////////////////////////////////////////////////////////////////////
/// \class Control                                            hantek/control.h
/// \brief The DsoControl abstraction layer for %Hantek USB DSOs.
class HantekDsoControl : public QObject {
    Q_OBJECT

public:
    /**
   * Creates a dsoControl object. The actual event loop / timer is not started.
   * You can optionally create a thread and move the created object to the thread.
   * You need to call updateInterval() to start the timer.
   * @param device
   */
    HantekDsoControl(USBDevice* device);
    ~HantekDsoControl();

    /// Call this to start the processing. This method will call itself periodically from there on.
    /// It is wise to move this class object to an own thread and call run from there.
    void run();

    unsigned int getChannelCount();
    QList<unsigned int> *getAvailableRecordLengths();
    double getMinSamplerate();
    double getMaxSamplerate();

    const QStringList *getSpecialTriggerSources();
    const USBDevice* getDevice() const;

signals:
    void samplingStarted(); ///< The oscilloscope started sampling/waiting for trigger
    void samplingStopped(); ///< The oscilloscope stopped sampling/waiting for trigger
    void statusMessage(const QString &message,
                       int timeout); ///< Status message about the oscilloscope
    void samplesAvailable(const std::vector<std::vector<double>> *data,
                          double samplerate, bool append,
                          QMutex *mutex); ///< New sample data is available

    void availableRecordLengthsChanged(const QList<unsigned int> &recordLengths); ///< The available record lengths, empty list for continuous
    void samplerateLimitsChanged(double minimum, double maximum); ///< The minimum or maximum samplerate has changed
    void recordLengthChanged(unsigned long duration); ///< The record length has changed
    void recordTimeChanged(double duration); ///< The record time duration has changed
    void samplerateChanged(double samplerate); ///< The samplerate has changed
    void samplerateSet(int mode, QList<double> sampleSteps); ///< The samplerate has changed

    void communicationError();

protected:
    void updateInterval();
    unsigned int calculateTriggerPoint(unsigned int value);
    int getCaptureState();
    int getSamples(bool process);
    double getBestSamplerate(double samplerate, bool fastRate = false,
                             bool maximum = false, unsigned int *downsampler = 0);
    unsigned int getSampleCount(bool *fastRate = 0);
    unsigned int updateRecordLength(unsigned int size);
    unsigned int updateSamplerate(unsigned int downsampler, bool fastRate);
    void restoreTargets();
    void updateSamplerateLimits();

    // Communication with device
    USBDevice *device; ///< The USB device for the oscilloscope
    bool sampling; ///< true, if the oscilloscope is taking samples

    QStringList specialTriggerSources; ///< Names of the special trigger sources

    DataArray<unsigned char> *command[Hantek::BULK_COUNT]; ///< Pointers to bulk commands, ready to be transmitted
    bool commandPending[Hantek::BULK_COUNT]; ///< true, when the command should be executed
    DataArray<unsigned char> *control[Hantek::CONTROLINDEX_COUNT]; ///< Pointers to control commands
    unsigned char controlCode[Hantek::CONTROLINDEX_COUNT]; ///< Request codes for control commands
    bool controlPending[Hantek::CONTROLINDEX_COUNT]; ///< true, when the control command should be executed

    // Device setup
    Hantek::ControlSpecification specification; ///< The specifications of the device
    Hantek::ControlSettings settings;           ///< The current settings of the device

    // Results
    std::vector<std::vector<double>> samples; ///< Sample data vectors sent to the data analyzer
    unsigned int previousSampleCount; ///< The expected total number of samples at
    ///the last check before sampling started
    QMutex samplesMutex;              ///< Mutex for the sample data

    // State of the communication thread
    int captureState = Hantek::CAPTURE_WAITING;
    int rollState = 0;
    bool _samplingStarted = false;
    Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode)-1;
    int cycleCounter=0;
    int startCycle=0;
    int cycleTime=0;
public slots:
    void startSampling();
    void stopSampling();

    unsigned int setRecordLength(unsigned int size);
    double setSamplerate(double samplerate = 0.0);
    double setRecordTime(double duration = 0.0);

    int setChannelUsed(unsigned int channel, bool used);
    int setCoupling(unsigned int channel, Dso::Coupling coupling);
    double setGain(unsigned int channel, double gain);
    double setOffset(unsigned int channel, double offset);

    int setTriggerMode(Dso::TriggerMode mode);
    int setTriggerSource(bool special, unsigned int id);
    double setTriggerLevel(unsigned int channel, double level);
    int setTriggerSlope(Dso::Slope slope);
    double setPretriggerPosition(double position);
    int forceTrigger();

#ifdef DEBUG
    int stringCommand(QString command);
#endif
};
