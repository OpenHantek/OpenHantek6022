// SPDX-License-Identifier: GPL-2.0+

#include <cmath>
#include <limits>
#include <vector>

#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QTimer>

#include "hantek/hantekdsocontrol.h"

#include "usb/usbdevice.h"
#include "utils/printutils.h"
#include <stdexcept>

using namespace Hantek;

/// \brief Start sampling process.
void HantekDsoControl::startSampling() {
    sampling = true;

    // Emit signals for initial settings
    emit availableRecordLengthsChanged(controlsettings.samplerate.limits->recordLengths);
    updateSamplerateLimits();
    emit recordLengthChanged(getRecordLength());
    if (!isRollMode()) emit recordTimeChanged((double)getRecordLength() / controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    if (specification.isSoftwareTriggerDevice) {
        // Convert to GUI presentable values (1e5 -> 1.0, 48e6 -> 480.0 etc)
        QList<double> sampleSteps;
        for (double v: specification.sampleSteps) {
            sampleSteps << v/1e5;
        }
        emit samplerateSet(1, sampleSteps);
    }

    emit samplingStarted();
}

/// \brief Stop sampling process.
void HantekDsoControl::stopSampling() {
    sampling = false;
    emit samplingStopped();
}

/// \brief Get a list of the names of the special trigger sources.
const QStringList *HantekDsoControl::getSpecialTriggerSources() { return &(specialTriggerSources); }

const USBDevice *HantekDsoControl::getDevice() const { return device; }

const DSOsamples &HantekDsoControl::getLastSamples() { return result; }

/// \brief Initializes the command buffers and lists.
/// \param parent The parent widget.
HantekDsoControl::HantekDsoControl(USBDevice *device) : device(device) {
    if (device == nullptr) throw new std::runtime_error("No usb device for HantekDsoControl");

    // Use DSO-2090 specification as default
    specification.command.bulk.setRecordLength = (BulkCode)-1;
    specification.command.bulk.setChannels = (BulkCode)-1;
    specification.command.bulk.setGain = (BulkCode)-1;
    specification.command.bulk.setSamplerate = (BulkCode)-1;
    specification.command.bulk.setTrigger = (BulkCode)-1;
    specification.command.bulk.setPretrigger = (BulkCode)-1;
    specification.command.control.setOffset = (ControlCode)-1;
    specification.command.control.setRelays = (ControlCode)-1;
    specification.command.values.offsetLimits = (ControlValue)-1;
    specification.command.values.voltageLimits = (ControlValue)-1;

    specification.samplerate.single.base = 50e6;
    specification.samplerate.single.max = 50e6;
    specification.samplerate.single.recordLengths << 0;
    specification.samplerate.multi.base = 100e6;
    specification.samplerate.multi.max = 100e6;
    specification.samplerate.multi.recordLengths << 0;

    for (unsigned channel = 0; channel < HANTEK_CHANNELS; ++channel) {
        for (unsigned gainId = 0; gainId < 9; ++gainId) {
            specification.offsetLimit[channel][gainId][OFFSET_START] = 0x0000;
            specification.offsetLimit[channel][gainId][OFFSET_END] = 0xffff;
        }
    }

    // Set settings to default values
    controlsettings.samplerate.limits = &(specification.samplerate.single);
    controlsettings.samplerate.downsampler = 1;
    controlsettings.samplerate.current = 1e8;
    controlsettings.trigger.position = 0;
    controlsettings.trigger.point = 0;
    controlsettings.trigger.mode = Dso::TRIGGERMODE_NORMAL;
    controlsettings.trigger.slope = Dso::SLOPE_POSITIVE;
    controlsettings.trigger.special = false;
    controlsettings.trigger.source = 0;
    for (unsigned channel = 0; channel < HANTEK_CHANNELS; ++channel) {
        controlsettings.trigger.level[channel] = 0.0;
        controlsettings.voltage[channel].gain = 0;
        controlsettings.voltage[channel].offset = 0.0;
        controlsettings.voltage[channel].offsetReal = 0.0;
        controlsettings.voltage[channel].used = false;
    }
    controlsettings.recordLengthId = 1;
    controlsettings.usedChannels = 0;

    // Transmission-ready control commands
    this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
    this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
    this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
    this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;

    for (int cIndex = 0; cIndex < CONTROLINDEX_COUNT; ++cIndex) this->controlPending[cIndex] = false;

    // Sample buffers
    result.data.resize(HANTEK_CHANNELS);

    int errorCode;

    // Instantiate the commands needed for all models
    command[BULK_FORCETRIGGER] = new BulkForceTrigger();
    command[BULK_STARTSAMPLING] = new BulkCaptureStart();
    command[BULK_ENABLETRIGGER] = new BulkTriggerEnabled();
    command[BULK_GETDATA] = new BulkGetData();
    command[BULK_GETCAPTURESTATE] = new BulkGetCaptureState();
    command[BULK_SETGAIN] = new BulkSetGain();
    // Initialize the command versions to the ones used on the DSO-2090
    specification.command.bulk.setRecordLength = (BulkCode)-1;
    specification.command.bulk.setChannels = (BulkCode)-1;
    specification.command.bulk.setGain = BULK_SETGAIN;
    specification.command.bulk.setSamplerate = (BulkCode)-1;
    specification.command.bulk.setTrigger = (BulkCode)-1;
    specification.command.bulk.setPretrigger = (BulkCode)-1;
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.values.offsetLimits = VALUE_OFFSETLIMITS;
    specification.command.values.voltageLimits = (ControlValue)-1;

    // Determine the command version we need for this model
    switch (device->getUniqueModelID()) {
    case MODEL_DSO2150:
        command[BULK_SETTRIGGERANDSAMPLERATE] = new BulkSetTriggerAndSamplerate();
        specification.command.bulk.setRecordLength = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setChannels = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setPretrigger = BULK_SETTRIGGERANDSAMPLERATE;
        for (int cIndex = 0; cIndex <= CONTROLINDEX_SETRELAYS; ++cIndex) this->controlPending[cIndex] = true;
        // Initialize those as pending
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        specification.samplerate.single.base = 50e6;
        specification.samplerate.single.max = 75e6;
        specification.samplerate.single.maxDownsampler = 131072;
        specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 32768;
        specification.samplerate.multi.base = 100e6;
        specification.samplerate.multi.max = 150e6;
        specification.samplerate.multi.maxDownsampler = 131072;
        specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 65536;
        specification.bufferDividers << 1000 << 1 << 1;
        specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 << 8.0 << 16.0 << 40.0;
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel)
            specification.voltageLimit[channel] << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255;
        specification.gainIndex << 0 << 1 << 2 << 0 << 1 << 2 << 0 << 1 << 2;
        specification.sampleSize = 8;
        break;
    case MODEL_DSO2090:
        command[BULK_SETTRIGGERANDSAMPLERATE] = new BulkSetTriggerAndSamplerate();
        specification.command.bulk.setRecordLength = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setChannels = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
        specification.command.bulk.setPretrigger = BULK_SETTRIGGERANDSAMPLERATE;
        for (int cIndex = 0; cIndex <= CONTROLINDEX_SETRELAYS; ++cIndex) this->controlPending[cIndex] = true;
        // Initialize those as pending
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        specification.samplerate.single.base = 50e6;
        specification.samplerate.single.max = 50e6;
        specification.samplerate.single.maxDownsampler = 131072;
        specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 32768;
        specification.samplerate.multi.base = 100e6;
        specification.samplerate.multi.max = 100e6;
        specification.samplerate.multi.maxDownsampler = 131072;
        specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 65536;
        specification.bufferDividers << 1000 << 1 << 1;
        specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 << 8.0 << 16.0 << 40.0;
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel)
            specification.voltageLimit[channel] << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255;
        specification.gainIndex << 0 << 1 << 2 << 0 << 1 << 2 << 0 << 1 << 2;
        specification.sampleSize = 8;
        break;

    case MODEL_DSO2250:
        // Instantiate additional commands for the DSO-2250
        command[BULK_BSETCHANNELS] = new BulkSetChannels2250();
        command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetTrigger2250();
        command[BULK_DSETBUFFER] = new BulkSetRecordLength2250();
        command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetSamplerate2250();
        command[BULK_FSETBUFFER] = new BulkSetBuffer2250();
        specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
        specification.command.bulk.setChannels = BULK_BSETCHANNELS;
        specification.command.bulk.setSamplerate = BULK_ESETTRIGGERORSAMPLERATE;
        specification.command.bulk.setTrigger = BULK_CSETTRIGGERORSAMPLERATE;
        specification.command.bulk.setPretrigger = BULK_FSETBUFFER;

        for (int cIndex = 0; cIndex <= CONTROLINDEX_SETRELAYS; ++cIndex) this->controlPending[cIndex] = true;

        commandPending[BULK_BSETCHANNELS] = true;
        commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
        commandPending[BULK_DSETBUFFER] = true;
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
        commandPending[BULK_FSETBUFFER] = true;

        specification.samplerate.single.base = 100e6;
        specification.samplerate.single.max = 100e6;
        specification.samplerate.single.maxDownsampler = 65536;
        specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 524288;
        specification.samplerate.multi.base = 200e6;
        specification.samplerate.multi.max = 250e6;
        specification.samplerate.multi.maxDownsampler = 65536;
        specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 1048576;
        specification.bufferDividers << 1000 << 1 << 1;
        specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 << 8.0 << 16.0 << 40.0;
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel)
            specification.voltageLimit[channel] << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255 << 255;
        specification.gainIndex << 0 << 2 << 3 << 0 << 2 << 3 << 0 << 2 << 3;
        specification.sampleSize = 8;
        break;

    case MODEL_DSO5200A:
        [[clang::fallthrough]];
    case MODEL_DSO5200:
        // Instantiate additional commands for the DSO-5200
        command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetSamplerate5200();
        command[BULK_DSETBUFFER] = new BulkSetBuffer5200();
        command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetTrigger5200();
        specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
        specification.command.bulk.setChannels = BULK_ESETTRIGGERORSAMPLERATE;
        specification.command.bulk.setSamplerate = BULK_CSETTRIGGERORSAMPLERATE;
        specification.command.bulk.setTrigger = BULK_ESETTRIGGERORSAMPLERATE;
        specification.command.bulk.setPretrigger = BULK_ESETTRIGGERORSAMPLERATE;
        // specification.command.values.voltageLimits = VALUE_ETSCORRECTION;
        for (int cIndex = 0; cIndex <= CONTROLINDEX_SETRELAYS; ++cIndex) this->controlPending[cIndex] = true;

        commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
        commandPending[BULK_DSETBUFFER] = true;
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

        specification.samplerate.single.base = 100e6;
        specification.samplerate.single.max = 125e6;
        specification.samplerate.single.maxDownsampler = 131072;
        specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 14336;
        specification.samplerate.multi.base = 200e6;
        specification.samplerate.multi.max = 250e6;
        specification.samplerate.multi.maxDownsampler = 131072;
        specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 28672;
        specification.bufferDividers << 1000 << 1 << 1;
        specification.gainSteps << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 << 8.0 << 16.0 << 40.0 << 80.0;
        /// \todo Use calibration data to get the DSO-5200(A) sample ranges
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel)
            specification.voltageLimit[channel] << 368 << 454 << 908 << 368 << 454 << 908 << 368 << 454 << 908;
        specification.gainIndex << 1 << 0 << 0 << 1 << 0 << 0 << 1 << 0 << 0;
        specification.sampleSize = 10;
        break;

    case MODEL_DSO6022BE:
        device->overwriteInPacketLength(16384);
        // 6022BE do not support any bulk commands
        device->setEnableBulkTransfer(false);
        specification.useControlNoBulk = true;
        specification.isSoftwareTriggerDevice = true;
        specification.supportsCaptureState = false;
        specification.supportsOffset = false;
        specification.supportsCouplingRelays = false;

        this->control[CONTROLINDEX_SETVOLTDIV_CH1] = new ControlSetVoltDIV_CH1();
        this->controlCode[CONTROLINDEX_SETVOLTDIV_CH1] = CONTROL_SETVOLTDIV_CH1;
        this->controlPending[CONTROLINDEX_SETVOLTDIV_CH1] = true;

        this->control[CONTROLINDEX_SETVOLTDIV_CH2] = new ControlSetVoltDIV_CH2();
        this->controlCode[CONTROLINDEX_SETVOLTDIV_CH2] = CONTROL_SETVOLTDIV_CH2;
        this->controlPending[CONTROLINDEX_SETVOLTDIV_CH2] = true;

        this->control[CONTROLINDEX_SETTIMEDIV] = new ControlSetTimeDIV();
        this->controlCode[CONTROLINDEX_SETTIMEDIV] = CONTROL_SETTIMEDIV;
        this->controlPending[CONTROLINDEX_SETTIMEDIV] = true;

        this->control[CONTROLINDEX_ACQUIIRE_HARD_DATA] = new ControlAcquireHardData();
        this->controlCode[CONTROLINDEX_ACQUIIRE_HARD_DATA] = CONTROL_ACQUIIRE_HARD_DATA;
        this->controlPending[CONTROLINDEX_ACQUIIRE_HARD_DATA] = true;

        this->controlPending[CONTROLINDEX_SETOFFSET] = false;
        this->controlPending[CONTROLINDEX_SETRELAYS] = false;

        specification.samplerate.single.base = 1e6;
        specification.samplerate.single.max = 48e6;
        specification.samplerate.single.maxDownsampler = 10;
        specification.samplerate.single.recordLengths << UINT_MAX << 10240;
        specification.samplerate.multi.base = 1e6;
        specification.samplerate.multi.max = 48e6;
        specification.samplerate.multi.maxDownsampler = 10;
        specification.samplerate.multi.recordLengths << UINT_MAX << 20480;
        specification.bufferDividers << 1000 << 1 << 1;
        specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 << 8.0 << 16.0 << 40.0;
        // This data was based on testing and depends on Divider.
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel)
            specification.voltageLimit[channel] << 25 << 51 << 103 << 206 << 412 << 196 << 392 << 784 << 1000;
        // Divider. Tested and calculated results are different!
        specification.gainDiv << 10 << 10 << 10 << 10 << 10 << 2 << 2 << 2 << 1;
        specification.sampleSteps << 1e5 << 2e5 << 5e5 << 1e6 << 2e6 << 4e6 << 8e6 << 16e6 << 24e6 << 48e6;
        specification.sampleDiv << 10 << 20 << 50 << 1 << 2 << 4 << 8 << 16 << 24 << 48;
        specification.sampleSize = 8;
        break;

    default:
        throw new std::runtime_error("unknown model");
    }

    controlsettings.recordLengthId = 1;
    controlsettings.samplerate.limits = &(specification.samplerate.single);
    controlsettings.samplerate.downsampler = 1;
    this->previousSampleCount = 0;

    // Get channel level data
    errorCode = device->controlRead(CONTROL_VALUE, (unsigned char *)&(specification.offsetLimit),
                                    sizeof(specification.offsetLimit), (int)VALUE_OFFSETLIMITS);
    if (errorCode < 0) {
        device->disconnect();
        emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
        return;
    }

    sampling = false;
}

/// \brief Disconnects the device.
HantekDsoControl::~HantekDsoControl() {
    // Clean up commands
    for (int cIndex = 0; cIndex < BULK_COUNT; ++cIndex) { delete command[cIndex]; }
}

/// \brief Gets the physical channel count for this oscilloscope.
/// \return The number of physical channels.
unsigned HantekDsoControl::getChannelCount() { return HANTEK_CHANNELS; }

/// \brief Get available record lengths for this oscilloscope.
/// \return The number of physical channels, empty list for continuous.
QList<unsigned> *HantekDsoControl::getAvailableRecordLengths() { return &controlsettings.samplerate.limits->recordLengths; }

/// \brief Get minimum samplerate for this oscilloscope.
/// \return The minimum samplerate for the current configuration in S/s.
double HantekDsoControl::getMinSamplerate() {
    return (double)specification.samplerate.single.base / specification.samplerate.single.maxDownsampler;
}

/// \brief Get maximum samplerate for this oscilloscope.
/// \return The maximum samplerate for the current configuration in S/s.
double HantekDsoControl::getMaxSamplerate() {
    ControlSamplerateLimits *limits =
        (controlsettings.usedChannels <= 1) ? &specification.samplerate.multi : &specification.samplerate.single;
    return limits->max;
}

/// \brief Updates the interval of the periodic thread timer.
void HantekDsoControl::updateInterval() {
    // Check the current oscilloscope state everytime 25% of the time the buffer
    // should be refilled
    if (isRollMode())
        cycleTime = (int)((double)device->getPacketSize() /
                          ((controlsettings.samplerate.limits == &specification.samplerate.multi) ? 1 : HANTEK_CHANNELS) /
                          controlsettings.samplerate.current * 250);
    else
        cycleTime = (int)((double)getRecordLength() / controlsettings.samplerate.current * 250);

    // Not more often than every 10 ms though but at least once every second
    cycleTime = qBound(10, cycleTime, 1000);
}

bool HantekDsoControl::isRollMode() {
    return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId] == UINT_MAX;
}

int HantekDsoControl::getRecordLength() { return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId]; }

/// \brief Calculates the trigger point from the CommandGetCaptureState data.
/// \param value The data value that contains the trigger point.
/// \return The calculated trigger point for the given data.
unsigned HantekDsoControl::calculateTriggerPoint(unsigned value) {
    unsigned result = value;

    // Each set bit inverts all bits with a lower value
    for (unsigned bitValue = 1; bitValue; bitValue <<= 1)
        if (result & bitValue) result ^= bitValue - 1;

    return result;
}

/// \brief Gets the current state.
/// \return The current CaptureState of the oscilloscope, libusb error code on
/// error.
int HantekDsoControl::getCaptureState() {
    int errorCode;

    if (!specification.supportsCaptureState) return CAPTURE_READY;

    errorCode = device->bulkCommand(command[BULK_GETCAPTURESTATE], 1);
    if (errorCode < 0) return errorCode;

    BulkResponseGetCaptureState response;
    errorCode = device->bulkRead(response.data(), response.getSize());
    if (errorCode < 0) return errorCode;

    controlsettings.trigger.point = this->calculateTriggerPoint(response.getTriggerPoint());

    return (int)response.getCaptureState();
}

/// \brief Gets sample data from the oscilloscope and converts it.
/// \return sample count on success, libusb error code on error.
/// TODO Refactor. MODEL_DSO6022BE needs to be handled differently most of the time
int HantekDsoControl::getSamples(bool process) {
    int errorCode;

    const unsigned DROP_DSO6022_HEAD = 0x410;
    const unsigned DROP_DSO6022_TAIL = 0x3F0;

    if (device->getUniqueModelID() != MODEL_DSO6022BE) {
        // Request data
        errorCode = device->bulkCommand(command[BULK_GETDATA], 1);
        if (errorCode < 0) return errorCode;
    }

    // Save raw data to temporary buffer
    bool fastRate = false;
    unsigned totalSampleCount = this->getSampleCount(&fastRate);
    if (totalSampleCount == UINT_MAX) return LIBUSB_ERROR_INVALID_PARAM;

    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if (totalSampleCount < this->previousSampleCount) {
        unsigned currentSampleCount = totalSampleCount;
        totalSampleCount = this->previousSampleCount;
        this->previousSampleCount = currentSampleCount; // Using sampleCount as temporary buffer since it
                                                        // was set to totalSampleCount
    } else {
        this->previousSampleCount = totalSampleCount;
    }

    unsigned sampleCount = totalSampleCount;
    if (!fastRate) sampleCount /= HANTEK_CHANNELS;
    unsigned dataLength = totalSampleCount;
    if (specification.sampleSize > 8) dataLength *= 2;

    std::vector<unsigned char> data(dataLength);
    errorCode = device->bulkReadMulti(data.data(), dataLength);
    if (errorCode < 0) return errorCode;

    // Process the data only if we want it
    if (!process) return LIBUSB_SUCCESS;

    // How much data did we really receive?
    dataLength = errorCode;
    if (specification.sampleSize > 8)
        totalSampleCount = dataLength / 2;
    else
        totalSampleCount = dataLength;

    // Convert channel data
    if (fastRate) {
        QWriteLocker locker(&result.lock);
        result.samplerate = controlsettings.samplerate.current;
        result.append = isRollMode();

        // Fast rate mode, one channel is using all buffers
        sampleCount = totalSampleCount;
        int channel = 0;
        for (; channel < HANTEK_CHANNELS; ++channel) {
            if (controlsettings.voltage[channel].used) break;
        }

        // Clear unused channels
        for (int channelCounter = 0; channelCounter < HANTEK_CHANNELS; ++channelCounter)
            if (channelCounter != channel) { result.data[channelCounter].clear(); }

        if (channel < HANTEK_CHANNELS) {
            // Resize sample vector
            result.data[channel].resize(sampleCount);

            // Convert data from the oscilloscope and write it into the sample
            // buffer
            unsigned bufferPosition = controlsettings.trigger.point * 2;
            if (specification.sampleSize > 8) {
                // Additional most significant bits after the normal data
                unsigned extraBitsPosition; // Track the position of the extra
                // bits in the additional byte
                unsigned extraBitsSize = specification.sampleSize - 8;                 // Number of extra bits
                unsigned short int extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction

                for (unsigned realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
                    if (bufferPosition >= sampleCount) bufferPosition %= sampleCount;

                    extraBitsPosition = bufferPosition % HANTEK_CHANNELS;

                    result.data[channel][realPosition] =
                        ((double)((unsigned short int)data[bufferPosition] +
                                  (((unsigned short int)data[sampleCount + bufferPosition - extraBitsPosition]
                                    << (8 - (HANTEK_CHANNELS - 1 - extraBitsPosition) * extraBitsSize)) &
                                   extraBitsMask)) /
                             specification.voltageLimit[channel][controlsettings.voltage[channel].gain] -
                         controlsettings.voltage[channel].offsetReal) *
                        specification.gainSteps[controlsettings.voltage[channel].gain];
                }
            } else {
                for (unsigned realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
                    if (bufferPosition >= sampleCount) bufferPosition %= sampleCount;

                    double dataBuf = (double)((int)data[bufferPosition]);
                    result.data[channel][realPosition] =
                        (dataBuf / specification.voltageLimit[channel][controlsettings.voltage[channel].gain] -
                         controlsettings.voltage[channel].offsetReal) *
                        specification.gainSteps[controlsettings.voltage[channel].gain];
                }
            }
        }
    } else {
        QWriteLocker locker(&result.lock);
        result.samplerate = controlsettings.samplerate.current;
        result.append = isRollMode();

        // Normal mode, channels are using their separate buffers
        sampleCount = totalSampleCount / HANTEK_CHANNELS;
        // if device is 6022BE, drop heading & trailing samples
        if (device->getUniqueModelID() == MODEL_DSO6022BE) sampleCount -= (DROP_DSO6022_HEAD + DROP_DSO6022_TAIL);
        for (int channel = 0; channel < HANTEK_CHANNELS; ++channel) {
            if (controlsettings.voltage[channel].used) {
                // Resize sample vector
                if (result.data[channel].size() < sampleCount) { result.data[channel].resize(sampleCount); }

                // Convert data from the oscilloscope and write it into the sample
                // buffer
                unsigned bufferPosition = controlsettings.trigger.point * 2;
                if (specification.sampleSize > 8) {
                    // Additional most significant bits after the normal data
                    unsigned extraBitsSize = specification.sampleSize - 8; // Number of extra bits
                    unsigned short int extraBitsMask =
                        (0x00ff << extraBitsSize) & 0xff00;    // Mask for extra bits extraction
                    unsigned extraBitsIndex = 8 - channel * 2; // Bit position offset for extra bits extraction

                    for (unsigned realPosition = 0; realPosition < sampleCount;
                         ++realPosition, bufferPosition += HANTEK_CHANNELS) {
                        if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                        result.data[channel][realPosition] =
                            ((double)((unsigned short int)data[bufferPosition + HANTEK_CHANNELS - 1 - channel] +
                                      (((unsigned short int)data[totalSampleCount + bufferPosition] << extraBitsIndex) &
                                       extraBitsMask)) /
                                 specification.voltageLimit[channel][controlsettings.voltage[channel].gain] -
                             controlsettings.voltage[channel].offsetReal) *
                            specification.gainSteps[controlsettings.voltage[channel].gain];
                    }
                } else {
                    if (device->getUniqueModelID() == MODEL_DSO6022BE) {
                        bufferPosition += channel;
                        // if device is 6022BE, offset DROP_DSO6022_HEAD incrementally
                        bufferPosition += DROP_DSO6022_HEAD * 2;
                    } else
                        bufferPosition += HANTEK_CHANNELS - 1 - channel;

                    for (unsigned realPosition = 0; realPosition < sampleCount;
                         ++realPosition, bufferPosition += HANTEK_CHANNELS) {
                        if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                        if (device->getUniqueModelID() == MODEL_DSO6022BE) {
                            double dataBuf = (double)((int)(data[bufferPosition] - 0x83));
                            result.data[channel][realPosition] =
                                (dataBuf / specification.voltageLimit[channel][controlsettings.voltage[channel].gain]) *
                                specification.gainSteps[controlsettings.voltage[channel].gain];
                        } else {
                            double dataBuf = (double)((int)(data[bufferPosition]));
                            result.data[channel][realPosition] =
                                (dataBuf / specification.voltageLimit[channel][controlsettings.voltage[channel].gain] -
                                 controlsettings.voltage[channel].offsetReal) *
                                specification.gainSteps[controlsettings.voltage[channel].gain];
                        }
                    }
                }
            } else {
                // Clear unused channels
                result.data[channel].clear();
            }
        }
    }

    static unsigned id = 0;
    ++id;
    timestampDebug(QString("Received packet %1").arg(id));

    emit samplesAvailable();

    return errorCode;
}

/// \brief Calculated the nearest samplerate supported by the oscilloscope.
/// \param samplerate The target samplerate, that should be met as good as
/// possible.
/// \param fastRate true, if the fast rate mode is enabled.
/// \param maximum The target samplerate is the maximum allowed when true, the
/// minimum otherwise.
/// \param downsampler Pointer to where the selected downsampling factor should
/// be written.
/// \return The nearest samplerate supported, 0.0 on error.
double HantekDsoControl::getBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned *downsampler) {
    // Abort if the input value is invalid
    if (samplerate <= 0.0) return 0.0;

    double bestSamplerate = 0.0;

    // Get samplerate specifications for this mode and model
    ControlSamplerateLimits *limits;
    if (fastRate)
        limits = &(specification.samplerate.multi);
    else
        limits = &(specification.samplerate.single);

    // Get downsampling factor that would provide the requested rate
    double bestDownsampler = (double)limits->base / specification.bufferDividers[controlsettings.recordLengthId] / samplerate;
    // Base samplerate sufficient, or is the maximum better?
    if (bestDownsampler < 1.0 &&
        (samplerate <= limits->max / specification.bufferDividers[controlsettings.recordLengthId] || !maximum)) {
        bestDownsampler = 0.0;
        bestSamplerate = limits->max / specification.bufferDividers[controlsettings.recordLengthId];
    } else {
        switch (specification.command.bulk.setSamplerate) {
        case BULK_SETTRIGGERANDSAMPLERATE:
            // DSO-2090 supports the downsampling factors 1, 2, 4 and 5 using
            // valueFast or all even values above using valueSlow
            if ((maximum && bestDownsampler <= 5.0) || (!maximum && bestDownsampler < 6.0)) {
                // valueFast is used
                if (maximum) {
                    // The samplerate shall not be higher, so we round up
                    bestDownsampler = ceil(bestDownsampler);
                    if (bestDownsampler > 2.0) // 3 and 4 not possible with the DSO-2090
                        bestDownsampler = 5.0;
                } else {
                    // The samplerate shall not be lower, so we round down
                    bestDownsampler = floor(bestDownsampler);
                    if (bestDownsampler > 2.0 && bestDownsampler < 5.0) // 3 and 4 not possible with the DSO-2090
                        bestDownsampler = 2.0;
                }
            } else {
                // valueSlow is used
                if (maximum) {
                    bestDownsampler = ceil(bestDownsampler / 2.0) * 2.0; // Round up to next even value
                } else {
                    bestDownsampler = floor(bestDownsampler / 2.0) * 2.0; // Round down to next even value
                }
                if (bestDownsampler > 2.0 * 0x10001) // Check for overflow
                    bestDownsampler = 2.0 * 0x10001;
            }
            break;

        case BULK_CSETTRIGGERORSAMPLERATE:
            // DSO-5200 may not supports all downsampling factors, requires testing
            if (maximum) {
                bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
            } else {
                bestDownsampler = floor(bestDownsampler); // Round down to next integer value
            }
            break;

        case BULK_ESETTRIGGERORSAMPLERATE:
            // DSO-2250 doesn't have a fast value, so it supports all downsampling
            // factors
            if (maximum) {
                bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
            } else {
                bestDownsampler = floor(bestDownsampler); // Round down to next integer value
            }
            break;

        default:
            return 0.0;
        }

        // Limit maximum downsampler value to avoid overflows in the sent commands
        if (bestDownsampler > limits->maxDownsampler) bestDownsampler = limits->maxDownsampler;

        bestSamplerate = limits->base / bestDownsampler / specification.bufferDividers[controlsettings.recordLengthId];
    }

    if (downsampler) *downsampler = (unsigned)bestDownsampler;
    return bestSamplerate;
}

/// \brief Get the count of samples that are expected returned by the scope.
/// \param fastRate Is set to the state of the fast rate mode when provided.
/// \return The total number of samples the scope should return.
unsigned HantekDsoControl::getSampleCount(bool *fastRate) {
    unsigned totalSampleCount = getRecordLength();
    bool fastRateEnabled = controlsettings.samplerate.limits == &specification.samplerate.multi;

    if (totalSampleCount == UINT_MAX) {
        // Roll mode
        const int packetSize = device->getPacketSize();
        if (packetSize < 0)
            totalSampleCount = UINT_MAX;
        else
            totalSampleCount = packetSize;
    } else {
        if (!fastRateEnabled) totalSampleCount *= HANTEK_CHANNELS;
    }
    if (fastRate) *fastRate = fastRateEnabled;
    return totalSampleCount;
}

/// \brief Sets the size of the sample buffer without updating dependencies.
/// \param index The record length index that should be set.
/// \return The record length that has been set, 0 on error.
unsigned HantekDsoControl::updateRecordLength(unsigned index) {
    if (index >= (unsigned)controlsettings.samplerate.limits->recordLengths.size()) return 0;

    switch (specification.command.bulk.setRecordLength) {
    case BULK_SETTRIGGERANDSAMPLERATE:
        // SetTriggerAndSamplerate bulk command for record length
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])->setRecordLength(index);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        break;

    case BULK_DSETBUFFER:
        if (specification.command.bulk.setPretrigger == BULK_FSETBUFFER) {
            // Pointers to needed commands
            BulkSetRecordLength2250 *commandSetRecordLength2250 =
                static_cast<BulkSetRecordLength2250 *>(command[BULK_DSETBUFFER]);

            commandSetRecordLength2250->setRecordLength(index);
        } else {
            // SetBuffer5200 bulk command for record length
            BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(command[BULK_DSETBUFFER]);

            commandSetBuffer5200->setUsedPre(DTRIGGERPOSITION_ON);
            commandSetBuffer5200->setUsedPost(DTRIGGERPOSITION_ON);
            commandSetBuffer5200->setRecordLength(index);
        }

        commandPending[BULK_DSETBUFFER] = true;

        break;

    default:
        return 0;
    }

    // Check if the divider has changed and adapt samplerate limits accordingly
    bool bDividerChanged = specification.bufferDividers[index] != specification.bufferDividers[controlsettings.recordLengthId];

    controlsettings.recordLengthId = index;

    if (bDividerChanged) {
        this->updateSamplerateLimits();

        // Samplerate dividers changed, recalculate it
        this->restoreTargets();
    }

    return controlsettings.samplerate.limits->recordLengths[index];
}

/// \brief Sets the samplerate based on the parameters calculated by
/// Control::getBestSamplerate.
/// \param downsampler The downsampling factor.
/// \param fastRate true, if one channel uses all buffers.
/// \return The downsampling factor that has been set.
unsigned HantekDsoControl::updateSamplerate(unsigned downsampler, bool fastRate) {
    // Get samplerate limits
    Hantek::ControlSamplerateLimits *limits =
        fastRate ? &specification.samplerate.multi : &specification.samplerate.single;

    // Set the calculated samplerate
    switch (specification.command.bulk.setSamplerate) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        short int downsamplerValue = 0;
        unsigned char samplerateId = 0;
        bool downsampling = false;

        if (downsampler <= 5) {
            // All dividers up to 5 are done using the special samplerate IDs
            if (downsampler == 0 && limits->base >= limits->max)
                samplerateId = 1;
            else if (downsampler <= 2)
                samplerateId = downsampler;
            else { // Downsampling factors 3 and 4 are not supported
                samplerateId = 3;
                downsampler = 5;
                downsamplerValue = (short int)0xffff;
            }
        } else {
            // For any dividers above the downsampling factor can be set directly
            downsampler &= ~0x0001; // Only even values possible
            downsamplerValue = (short int)(0x10001 - (downsampler >> 1));

            downsampling = true;
        }

        // Pointers to needed commands
        BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate =
            static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE]);

        // Store if samplerate ID or downsampling factor is used
        commandSetTriggerAndSamplerate->setDownsamplingMode(downsampling);
        // Store samplerate ID
        commandSetTriggerAndSamplerate->setSamplerateId(samplerateId);
        // Store downsampling factor
        commandSetTriggerAndSamplerate->setDownsampler(downsamplerValue);
        // Set fast rate when used
        commandSetTriggerAndSamplerate->setFastRate(false /*fastRate*/);

        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        break;
    }
    case BULK_CSETTRIGGERORSAMPLERATE: {
        // Split the resulting divider into the values understood by the device
        // The fast value is kept at 4 (or 3) for slow sample rates
        long int valueSlow = qMax(((long int)downsampler - 3) / 2, (long int)0);
        unsigned char valueFast = downsampler - valueSlow * 2;

        // Pointers to needed commands
        BulkSetSamplerate5200 *commandSetSamplerate5200 =
            static_cast<BulkSetSamplerate5200 *>(command[BULK_CSETTRIGGERORSAMPLERATE]);
        BulkSetTrigger5200 *commandSetTrigger5200 =
            static_cast<BulkSetTrigger5200 *>(command[BULK_ESETTRIGGERORSAMPLERATE]);

        // Store samplerate fast value
        commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
        // Store samplerate slow value (two's complement)
        commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
        // Set fast rate when used
        commandSetTrigger5200->setFastRate(fastRate);

        commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // Pointers to needed commands
        BulkSetSamplerate2250 *commandSetSamplerate2250 =
            static_cast<BulkSetSamplerate2250 *>(command[BULK_ESETTRIGGERORSAMPLERATE]);

        bool downsampling = downsampler >= 1;
        // Store downsampler state value
        commandSetSamplerate2250->setDownsampling(downsampling);
        // Store samplerate value
        commandSetSamplerate2250->setSamplerate(downsampler > 1 ? 0x10001 - downsampler : 0);
        // Set fast rate when used
        commandSetSamplerate2250->setFastRate(fastRate);

        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

        break;
    }
    default:
        return UINT_MAX;
    }

    // Update settings
    bool fastRateChanged = fastRate != (controlsettings.samplerate.limits == &specification.samplerate.multi);
    if (fastRateChanged) { controlsettings.samplerate.limits = limits; }

    controlsettings.samplerate.downsampler = downsampler;
    if (downsampler)
        controlsettings.samplerate.current =
            controlsettings.samplerate.limits->base / specification.bufferDividers[controlsettings.recordLengthId] / downsampler;
    else
        controlsettings.samplerate.current =
            controlsettings.samplerate.limits->max / specification.bufferDividers[controlsettings.recordLengthId];

    // Update dependencies
    this->setPretriggerPosition(controlsettings.trigger.position);

    // Emit signals for changed settings
    if (fastRateChanged) {
        emit availableRecordLengthsChanged(controlsettings.samplerate.limits->recordLengths);
        emit recordLengthChanged(getRecordLength());
    }

    // Check for Roll mode
    if (!isRollMode()) emit recordTimeChanged((double)getRecordLength() / controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    return downsampler;
}

/// \brief Restore the samplerate/timebase targets after divider updates.
void HantekDsoControl::restoreTargets() {
    if (controlsettings.samplerate.target.samplerateSet)
        this->setSamplerate();
    else
        this->setRecordTime();
}

/// \brief Update the minimum and maximum supported samplerate.
void HantekDsoControl::updateSamplerateLimits() {
    // Works only if the minimum samplerate for normal mode is lower than for fast
    // rate mode, which is the case for all models
    ControlSamplerateLimits *limits =
        (controlsettings.usedChannels <= 1) ? &specification.samplerate.multi : &specification.samplerate.single;
    emit samplerateLimitsChanged((double)specification.samplerate.single.base /
                                     specification.samplerate.single.maxDownsampler /
                                     specification.bufferDividers[controlsettings.recordLengthId],
                                 limits->max / specification.bufferDividers[controlsettings.recordLengthId]);
}

/// \brief Sets the size of the oscilloscopes sample buffer.
/// \param index The record length index that should be set.
/// \return The record length that has been set, 0 on error.
unsigned HantekDsoControl::setRecordLength(unsigned index) {
    if (!device->isConnected()) return 0;

    if (!this->updateRecordLength(index)) return 0;

    this->restoreTargets();
    this->setPretriggerPosition(controlsettings.trigger.position);

    emit recordLengthChanged(getRecordLength());
    return getRecordLength();
}

/// \brief Sets the samplerate of the oscilloscope.
/// \param samplerate The samplerate that should be met (S/s), 0.0 to restore
/// current samplerate.
/// \return The samplerate that has been set, 0.0 on error.
double HantekDsoControl::setSamplerate(double samplerate) {
    if (!device->isConnected()) return 0.0;

    if (samplerate == 0.0) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = true;
    }

    if (!specification.isSoftwareTriggerDevice) {
        // When possible, enable fast rate if it is required to reach the requested
        // samplerate
        bool fastRate =
            (controlsettings.usedChannels <= 1) &&
            (samplerate > specification.samplerate.single.max / specification.bufferDividers[controlsettings.recordLengthId]);

        // What is the nearest, at least as high samplerate the scope can provide?
        unsigned downsampler = 0;
        double bestSamplerate = getBestSamplerate(samplerate, fastRate, false, &(downsampler));

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return 0.0;
        else {
            return bestSamplerate;
        }
    } else {
        int sampleId;
        for (sampleId = 0; sampleId < specification.sampleSteps.count() - 1; ++sampleId)
            if (specification.sampleSteps[sampleId] == samplerate) break;
        this->controlCode[CONTROLINDEX_SETTIMEDIV] = CONTROL_SETTIMEDIV;
        static_cast<ControlSetTimeDIV *>(this->control[CONTROLINDEX_SETTIMEDIV])
            ->setDiv(specification.sampleDiv[sampleId]);
        this->controlPending[CONTROLINDEX_SETTIMEDIV] = true;
        controlsettings.samplerate.current = samplerate;

        // Provide margin for SW trigger
        unsigned sampleMargin = 2000;
        // Check for Roll mode
        if (!isRollMode())
            emit recordTimeChanged((double)(getRecordLength() - sampleMargin) / controlsettings.samplerate.current);
        emit samplerateChanged(controlsettings.samplerate.current);

        return samplerate;
    }
}

/// \brief Sets the time duration of one aquisition by adapting the samplerate.
/// \param duration The record time duration that should be met (s), 0.0 to
/// restore current record time.
/// \return The record time duration that has been set, 0.0 on error.
double HantekDsoControl::setRecordTime(double duration) {
    if (!device->isConnected()) return 0.0;

    if (duration == 0.0) {
        duration = controlsettings.samplerate.target.duration;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = false;
    }

    if (!specification.isSoftwareTriggerDevice) {
        // Calculate the maximum samplerate that would still provide the requested
        // duration
        double maxSamplerate =
            (double)specification.samplerate.single.recordLengths[controlsettings.recordLengthId] / duration;

        // When possible, enable fast rate if the record time can't be set that low
        // to improve resolution
        bool fastRate = (controlsettings.usedChannels <= 1) &&
                        (maxSamplerate >=
                         specification.samplerate.multi.base / specification.bufferDividers[controlsettings.recordLengthId]);

        // What is the nearest, at most as high samplerate the scope can provide?
        unsigned downsampler = 0;
        double bestSamplerate = getBestSamplerate(maxSamplerate, fastRate, true, &(downsampler));

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return 0.0;
        else {
            return (double)getRecordLength() / bestSamplerate;
        }
    } else {
        // For now - we go for the 10240 size sampling - the other seems not to be
        // supported
        // Find highest samplerate using less than 10240 samples to obtain our
        // duration.
        // Better add some margin for our SW trigger
        unsigned sampleMargin = 2000;
        unsigned sampleCount = 10240;
        int bestId = 0;
        int sampleId;
        for (sampleId = 0; sampleId < specification.sampleSteps.count(); ++sampleId) {
            if (specification.sampleSteps[sampleId] * duration < (sampleCount - sampleMargin)) bestId = sampleId;
        }
        sampleId = bestId;
        // Usable sample value
        this->controlCode[CONTROLINDEX_SETTIMEDIV] = CONTROL_SETTIMEDIV;
        static_cast<ControlSetTimeDIV *>(this->control[CONTROLINDEX_SETTIMEDIV])
            ->setDiv(specification.sampleDiv[sampleId]);
        this->controlPending[CONTROLINDEX_SETTIMEDIV] = true;
        controlsettings.samplerate.current = specification.sampleSteps[sampleId];

        emit samplerateChanged(controlsettings.samplerate.current);
        return controlsettings.samplerate.current;
    }
}

/// \brief Enables/disables filtering of the given channel.
/// \param channel The channel that should be set.
/// \param used true if the channel should be sampled.
/// \return See ::Dso::ErrorCode.
int HantekDsoControl::setChannelUsed(unsigned channel, bool used) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ERROR_PARAMETER;

    // Update settings
    controlsettings.voltage[channel].used = used;
    unsigned channelCount = 0;
    for (int channelCounter = 0; channelCounter < HANTEK_CHANNELS; ++channelCounter) {
        if (controlsettings.voltage[channelCounter].used) ++channelCount;
    }

    // Calculate the UsedChannels field for the command
    unsigned char usedChannels = USED_CH1;

    if (controlsettings.voltage[1].used) {
        if (controlsettings.voltage[0].used) {
            usedChannels = USED_CH1CH2;
        } else {
            // DSO-2250 uses a different value for channel 2
            if (specification.command.bulk.setChannels == BULK_BSETCHANNELS)
                usedChannels = BUSED_CH2;
            else
                usedChannels = USED_CH2;
        }
    }

    switch (specification.command.bulk.setChannels) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger source
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])
            ->setUsedChannels(usedChannels);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
        break;
    }
    case BULK_BSETCHANNELS: {
        // SetChannels2250 bulk command for active channels
        static_cast<BulkSetChannels2250 *>(command[BULK_BSETCHANNELS])->setUsedChannels(usedChannels);
        commandPending[BULK_BSETCHANNELS] = true;

        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200s bulk command for trigger source
        static_cast<BulkSetTrigger5200 *>(command[BULK_ESETTRIGGERORSAMPLERATE])->setUsedChannels(usedChannels);
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
        break;
    }
    default:
        break;
    }

    // Check if fast rate mode availability changed
    bool fastRateChanged = (controlsettings.usedChannels <= 1) != (channelCount <= 1);
    controlsettings.usedChannels = channelCount;

    if (fastRateChanged) this->updateSamplerateLimits();

    return Dso::ERROR_NONE;
}

/// \brief Set the coupling for the given channel.
/// \param channel The channel that should be set.
/// \param coupling The new coupling for the channel.
/// \return See ::Dso::ErrorCode.
int HantekDsoControl::setCoupling(unsigned channel, Dso::Coupling coupling) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ERROR_PARAMETER;

    // SetRelays control command for coupling relays
    if (specification.supportsCouplingRelays) {
        static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS])
            ->setCoupling(channel, coupling != Dso::COUPLING_AC);
        this->controlPending[CONTROLINDEX_SETRELAYS] = true;
    }

    return Dso::ERROR_NONE;
}

/// \brief Sets the gain for the given channel.
/// \param channel The channel that should be set.
/// \param gain The gain that should be met (V/div).
/// \return The gain that has been set, ::Dso::ErrorCode on error.
double HantekDsoControl::setGain(unsigned channel, double gain) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ERROR_PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    int gainId;
    for (gainId = 0; gainId < specification.gainSteps.count() - 1; ++gainId)
        if (specification.gainSteps[gainId] >= gain) break;

    if (specification.useControlNoBulk) {
        if (channel == 0) {
            static_cast<ControlSetVoltDIV_CH1 *>(this->control[CONTROLINDEX_SETVOLTDIV_CH1])
                ->setDiv(specification.gainDiv[gainId]);
            this->controlPending[CONTROLINDEX_SETVOLTDIV_CH1] = true;
        } else if (channel == 1) {
            static_cast<ControlSetVoltDIV_CH2 *>(this->control[CONTROLINDEX_SETVOLTDIV_CH2])
                ->setDiv(specification.gainDiv[gainId]);
            this->controlPending[CONTROLINDEX_SETVOLTDIV_CH2] = true;
        } else
            qDebug("%s: Unsuported channel: %i\n", __func__, channel);
    } else {
        // SetGain bulk command for gain
        static_cast<BulkSetGain *>(command[BULK_SETGAIN])->setGain(channel, specification.gainIndex[gainId]);
        commandPending[BULK_SETGAIN] = true;

        // SetRelays control command for gain relays
        ControlSetRelays *controlSetRelays = static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS]);
        controlSetRelays->setBelow1V(channel, gainId < 3);
        controlSetRelays->setBelow100mV(channel, gainId < 6);
        this->controlPending[CONTROLINDEX_SETRELAYS] = true;
    }

    controlsettings.voltage[channel].gain = gainId;

    this->setOffset(channel, controlsettings.voltage[channel].offset);

    return specification.gainSteps[gainId];
}

/// \brief Set the offset for the given channel.
/// \param channel The channel that should be set.
/// \param offset The new offset value (0.0 - 1.0).
/// \return The offset that has been set, ::Dso::ErrorCode on error.
double HantekDsoControl::setOffset(unsigned channel, double offset) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ERROR_PARAMETER;

    // Calculate the offset value
    // The range is given by the calibration data (convert from big endian)
    unsigned short int minimum =
        ((unsigned short int)*(
             (unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_START]))
         << 8) +
        *((unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_START]) + 1);
    unsigned short int maximum =
        ((unsigned short int)*(
             (unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_END]))
         << 8) +
        *((unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_END]) + 1);
    unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
    double offsetReal = (double)(offsetValue - minimum) / (maximum - minimum);

    if (specification.supportsOffset) {
        static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setChannel(channel, offsetValue);
        this->controlPending[CONTROLINDEX_SETOFFSET] = true;
    }

    controlsettings.voltage[channel].offset = offset;
    controlsettings.voltage[channel].offsetReal = offsetReal;

    this->setTriggerLevel(channel, controlsettings.trigger.level[channel]);

    return offsetReal;
}

/// \brief Set the trigger mode.
/// \return See ::Dso::ErrorCode.
int HantekDsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (mode < Dso::TRIGGERMODE_AUTO || mode >= Dso::TRIGGERMODE_COUNT) return Dso::ERROR_PARAMETER;

    controlsettings.trigger.mode = mode;
    return Dso::ERROR_NONE;
}

/// \brief Set the trigger source.
/// \param special true for a special channel (EXT, ...) as trigger source.
/// \param id The number of the channel, that should be used as trigger.
/// \return See ::Dso::ErrorCode.
int HantekDsoControl::setTriggerSource(bool special, unsigned id) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if ((!special && id >= HANTEK_CHANNELS) || (special && id >= HANTEK_SPECIAL_CHANNELS)) return Dso::ERROR_PARAMETER;

    switch (specification.command.bulk.setTrigger) {
    case BULK_SETTRIGGERANDSAMPLERATE:
        // SetTriggerAndSamplerate bulk command for trigger source
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])
            ->setTriggerSource(special ? 3 + id : 1 - id);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
        break;

    case BULK_CSETTRIGGERORSAMPLERATE:
        // SetTrigger2250 bulk command for trigger source
        static_cast<BulkSetTrigger2250 *>(command[BULK_CSETTRIGGERORSAMPLERATE])
            ->setTriggerSource(special ? 0 : 2 + id);
        commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
        break;

    case BULK_ESETTRIGGERORSAMPLERATE:
        // SetTrigger5200 bulk command for trigger source
        static_cast<BulkSetTrigger5200 *>(command[BULK_ESETTRIGGERORSAMPLERATE])
            ->setTriggerSource(special ? 3 + id : 1 - id);
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
        break;

    default:
        return Dso::ERROR_UNSUPPORTED;
    }

    // SetRelays control command for external trigger relay
    static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS])->setTrigger(special);
    this->controlPending[CONTROLINDEX_SETRELAYS] = true;

    controlsettings.trigger.special = special;
    controlsettings.trigger.source = id;

    // Apply trigger level of the new source
    if (special) {
        // SetOffset control command for changed trigger level
        static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setTrigger(0x7f);
        this->controlPending[CONTROLINDEX_SETOFFSET] = true;
    } else
        this->setTriggerLevel(id, controlsettings.trigger.level[id]);

    return Dso::ERROR_NONE;
}

/// \brief Set the trigger level.
/// \param channel The channel that should be set.
/// \param level The new trigger level (V).
/// \return The trigger level that has been set, ::Dso::ErrorCode on error.
double HantekDsoControl::setTriggerLevel(unsigned channel, double level) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ERROR_PARAMETER;

    // Calculate the trigger level value
    unsigned short int minimum, maximum;
    if (specification.sampleSize>8) {
        // The range is the same as used for the offsets for 10 bit models
        minimum =
            ((unsigned short int)*(
                 (unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_START]))
             << 8) +
            *((unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_START]) + 1);
        maximum =
            ((unsigned short int)*(
                 (unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_END]))
             << 8) +
            *((unsigned char *)&(specification.offsetLimit[channel][controlsettings.voltage[channel].gain][OFFSET_END]) + 1);
    } else {
        // It's from 0x00 to 0xfd for the 8 bit models
        minimum = 0x00;
        maximum = 0xfd;
    }

    // Never get out of the limits
    unsigned short int levelValue =
        qBound((long int)minimum,
               (long int)((controlsettings.voltage[channel].offsetReal +
                           level / specification.gainSteps[controlsettings.voltage[channel].gain]) *
                              (maximum - minimum) +
                          0.5) +
                   minimum,
               (long int)maximum);

    // Check if the set channel is the trigger source
    if (!controlsettings.trigger.special && channel == controlsettings.trigger.source && specification.supportsOffset) {
        // SetOffset control command for trigger level
        static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setTrigger(levelValue);
        this->controlPending[CONTROLINDEX_SETOFFSET] = true;
    }

    /// \todo Get alternating trigger in here

    controlsettings.trigger.level[channel] = level;
    return (double)((levelValue - minimum) / (maximum - minimum) - controlsettings.voltage[channel].offsetReal) *
           specification.gainSteps[controlsettings.voltage[channel].gain];
}

/// \brief Set the trigger slope.
/// \param slope The Slope that should cause a trigger.
/// \return See ::Dso::ErrorCode.
int HantekDsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    if (slope >= Dso::SLOPE_COUNT) return Dso::ERROR_PARAMETER;

    switch (specification.command.bulk.setTrigger) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger slope
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerSlope(slope);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
        break;
    }
    case BULK_CSETTRIGGERORSAMPLERATE: {
        // SetTrigger2250 bulk command for trigger slope
        static_cast<BulkSetTrigger2250 *>(command[BULK_CSETTRIGGERORSAMPLERATE])->setTriggerSlope(slope);
        commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200 bulk command for trigger slope
        static_cast<BulkSetTrigger5200 *>(command[BULK_ESETTRIGGERORSAMPLERATE])->setTriggerSlope(slope);
        commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
        break;
    }
    default:
        return Dso::ERROR_UNSUPPORTED;
    }

    controlsettings.trigger.slope = slope;
    return Dso::ERROR_NONE;
}

int HantekDsoControl::forceTrigger() {
    commandPending[BULK_FORCETRIGGER] = true;
    return 0;
}

/// \brief Set the trigger position.
/// \param position The new trigger position (in s).
/// \return The trigger position that has been set.
double HantekDsoControl::setPretriggerPosition(double position) {
    if (!device->isConnected()) return -2;

    // All trigger positions are measured in samples
    unsigned positionSamples = position * controlsettings.samplerate.current;
    unsigned recordLength = getRecordLength();
    // Fast rate mode uses both channels
    if (controlsettings.samplerate.limits == &specification.samplerate.multi) positionSamples /= HANTEK_CHANNELS;

    switch (specification.command.bulk.setPretrigger) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        // Calculate the position value (Start point depending on record length)
        unsigned position = isRollMode() ? 0x1 : 0x7ffff - recordLength + positionSamples;

        // SetTriggerAndSamplerate bulk command for trigger position
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(position);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        break;
    }
    case BULK_FSETBUFFER: {
        // Calculate the position values (Inverse, maximum is 0x7ffff)
        unsigned positionPre = 0x7ffff - recordLength + positionSamples;
        unsigned positionPost = 0x7ffff - positionSamples;

        // SetBuffer2250 bulk command for trigger position
        BulkSetBuffer2250 *commandSetBuffer2250 = static_cast<BulkSetBuffer2250 *>(command[BULK_FSETBUFFER]);
        commandSetBuffer2250->setTriggerPositionPre(positionPre);
        commandSetBuffer2250->setTriggerPositionPost(positionPost);
        commandPending[BULK_FSETBUFFER] = true;

        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // Calculate the position values (Inverse, maximum is 0xffff)
        unsigned short int positionPre = 0xffff - recordLength + positionSamples;
        unsigned short int positionPost = 0xffff - positionSamples;

        // SetBuffer5200 bulk command for trigger position
        BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(command[BULK_DSETBUFFER]);
        commandSetBuffer5200->setTriggerPositionPre(positionPre);
        commandSetBuffer5200->setTriggerPositionPost(positionPost);
        commandPending[BULK_DSETBUFFER] = true;

        break;
    }
    default:
        return Dso::ERROR_UNSUPPORTED;
    }

    controlsettings.trigger.position = position;
    return (double)positionSamples / controlsettings.samplerate.current;
}

int HantekDsoControl::stringCommand(const QString &commandString) {
    if (!device->isConnected()) return Dso::ERROR_CONNECTION;

    QStringList commandParts = commandString.split(' ', QString::SkipEmptyParts);

    if (commandParts.count() < 1) return Dso::ERROR_PARAMETER;

    if (commandParts[0] != "send") return Dso::ERROR_UNSUPPORTED;

    if (commandParts.count() < 2) return Dso::ERROR_PARAMETER;

    if (commandParts[1] == "bulk") {
        QString data = commandString.section(' ', 2, -1, QString::SectionSkipEmpty);
        unsigned char commandCode = 0;

        // Read command code (First byte)
        hexParse(commandParts[2], &commandCode, 1);
        if (commandCode > BULK_COUNT) return Dso::ERROR_UNSUPPORTED;

        // Update bulk command and mark as pending
        hexParse(data, command[commandCode]->data(), command[commandCode]->getSize());
        commandPending[commandCode] = true;
        return Dso::ERROR_NONE;
    } else if (commandParts[1] == "control") {
        unsigned char controlCode = 0;

        // Read command code (First byte)
        hexParse(commandParts[2], &controlCode, 1);
        int cIndex;
        for (cIndex = 0; cIndex < CONTROLINDEX_COUNT; ++cIndex) {
            if (this->controlCode[cIndex] == controlCode) break;
        }
        if (cIndex >= CONTROLINDEX_COUNT) return Dso::ERROR_UNSUPPORTED;

        QString data = commandString.section(' ', 3, -1, QString::SectionSkipEmpty);

        // Update control command and mark as pending
        hexParse(data, this->control[cIndex]->data(), this->control[cIndex]->getSize());
        this->controlPending[cIndex] = true;
        return Dso::ERROR_NONE;
    } else
        return Dso::ERROR_UNSUPPORTED;
}

void HantekDsoControl::run() {
    int errorCode = 0;

    // Send all pending bulk commands
    for (int cIndex = 0; cIndex < BULK_COUNT; ++cIndex) {
        if (!commandPending[cIndex]) continue;

        timestampDebug(
            QString("Sending bulk command:%1").arg(hexDump(command[cIndex]->data(), command[cIndex]->getSize())));

        errorCode = device->bulkCommand(command[cIndex]);
        if (errorCode < 0) {
            qWarning("Sending bulk command %02x failed: %s", cIndex, libUsbErrorString(errorCode).toLocal8Bit().data());

            if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                emit communicationError();
                return;
            }
        } else
            commandPending[cIndex] = false;
    }

    // Send all pending control commands
    for (int cIndex = 0; cIndex < CONTROLINDEX_COUNT; ++cIndex) {
        if (!this->controlPending[cIndex]) continue;

        timestampDebug(QString("Sending control command %1:%2")
                           .arg(QString::number(this->controlCode[control], 16),
                                hexDump(this->control[control]->data(), this->control[control]->getSize())));

        errorCode = device->controlWrite(this->controlCode[cIndex], this->control[cIndex]->data(),
                                         this->control[cIndex]->getSize());
        if (errorCode < 0) {
            qWarning("Sending control command %2x failed: %s", this->controlCode[cIndex],
                     libUsbErrorString(errorCode).toLocal8Bit().data());

            if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                emit communicationError();
                return;
            }
        } else
            this->controlPending[cIndex] = false;
    }

    // State machine for the device communication
    if (isRollMode()) {
        // Roll mode
        this->captureState = CAPTURE_WAITING;
        bool toNextState = true;

        switch (this->rollState) {
        case ROLL_STARTSAMPLING:
            // Don't iterate through roll mode steps when stopped
            if (!this->sampling) {
                toNextState = false;
                break;
            }

            // Sampling hasn't started, update the expected sample count
            this->previousSampleCount = this->getSampleCount();

            errorCode = device->bulkCommand(command[BULK_STARTSAMPLING]);
            if (errorCode < 0) {
                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit communicationError();
                    return;
                }
                break;
            }

            timestampDebug("Starting to capture");

            this->_samplingStarted = true;

            break;

        case ROLL_ENABLETRIGGER:
            errorCode = device->bulkCommand(command[BULK_ENABLETRIGGER]);
            if (errorCode < 0) {
                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit communicationError();
                    return;
                }
                break;
            }

            timestampDebug("Enabling trigger");

            break;

        case ROLL_FORCETRIGGER:
            errorCode = device->bulkCommand(command[BULK_FORCETRIGGER]);
            if (errorCode < 0) {
                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit communicationError();
                    return;
                }
                break;
            }

            timestampDebug("Forcing trigger");

            break;

        case ROLL_GETDATA:
            // Get data and process it, if we're still sampling
            errorCode = this->getSamples(this->_samplingStarted);
            if (errorCode < 0)
                qWarning("Getting sample data failed: %s", libUsbErrorString(errorCode).toLocal8Bit().data());
            else
                timestampDebug(QString("Received %1 B of sampling data").arg(errorCode));

            // Check if we're in single trigger mode
            if (controlsettings.trigger.mode == Dso::TRIGGERMODE_SINGLE && this->_samplingStarted) this->stopSampling();

            // Sampling completed, restart it when necessary
            this->_samplingStarted = false;

            break;

        default:
            timestampDebug("Roll mode state unknown");
            break;
        }

        // Go to next state, or restart if last state was reached
        if (toNextState) this->rollState = (this->rollState + 1) % ROLL_COUNT;
    } else {
        // Standard mode
        this->rollState = ROLL_STARTSAMPLING;

        const int lastCaptureState = this->captureState;
        this->captureState = this->getCaptureState();
        if (this->captureState < 0)
            qWarning("Getting capture state failed: %s", libUsbErrorString(this->captureState).toLocal8Bit().data());

        else if (this->captureState != lastCaptureState)
            timestampDebug(QString("Capture state changed to %1").arg(this->captureState));

        switch (this->captureState) {
        case CAPTURE_READY:
        case CAPTURE_READY2250:
        case CAPTURE_READY5200:
            // Get data and process it, if we're still sampling
            errorCode = this->getSamples(this->_samplingStarted);
            if (errorCode < 0)
                qWarning("Getting sample data failed: %s", libUsbErrorString(errorCode).toLocal8Bit().data());
            else
                timestampDebug(QString("Received %1 B of sampling data").arg(errorCode));

            // Check if we're in single trigger mode
            if (controlsettings.trigger.mode == Dso::TRIGGERMODE_SINGLE && this->_samplingStarted) this->stopSampling();

            // Sampling completed, restart it when necessary
            this->_samplingStarted = false;

            // Start next capture if necessary by leaving out the break statement
            if (!this->sampling) break;

        case CAPTURE_WAITING:
            // Sampling hasn't started, update the expected sample count
            this->previousSampleCount = this->getSampleCount();

            if (this->_samplingStarted && this->lastTriggerMode == controlsettings.trigger.mode) {
                ++this->cycleCounter;

                if (this->cycleCounter == this->startCycle && !isRollMode()) {
                    // Buffer refilled completely since start of sampling, enable the
                    // trigger now
                    errorCode = device->bulkCommand(command[BULK_ENABLETRIGGER]);
                    if (errorCode < 0) {
                        if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                            emit communicationError();
                            return;
                        }
                        break;
                    }

                    timestampDebug("Enabling trigger");
                } else if (this->cycleCounter >= 8 + this->startCycle &&
                           controlsettings.trigger.mode == Dso::TRIGGERMODE_AUTO) {
                    // Force triggering
                    errorCode = device->bulkCommand(command[BULK_FORCETRIGGER]);
                    if (errorCode < 0) {
                        if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                            emit communicationError();
                            return;
                        }
                        break;
                    }

                    timestampDebug("Forcing trigger");
                }

                if (this->cycleCounter < 20 || this->cycleCounter < 4000 / cycleTime) break;
            }

            // Start capturing
            errorCode = device->bulkCommand(command[BULK_STARTSAMPLING]);
            if (errorCode < 0) {
                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit communicationError();
                    return;
                }
                break;
            }

            timestampDebug("Starting to capture");

            this->_samplingStarted = true;
            this->cycleCounter = 0;
            this->startCycle = controlsettings.trigger.position * 1000 / cycleTime + 1;
            this->lastTriggerMode = controlsettings.trigger.mode;
            break;

        case CAPTURE_SAMPLING:
            break;
        default:
            break;
        }
    }

    this->updateInterval();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    QTimer::singleShot(cycleTime, this, &HantekDsoControl::run);
#else
    QTimer::singleShot(cycleTime, this, SLOT(run()));
#endif
}
