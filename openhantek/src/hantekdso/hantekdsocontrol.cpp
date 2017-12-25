// SPDX-License-Identifier: GPL-2.0+

#include <cmath>
#include <limits>
#include <vector>
#include <assert.h>

#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QTimer>

#include "hantekdsocontrol.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "models/modelDSO6022.h"
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
        for (double v : specification.sampleSteps) { sampleSteps << v / 1e5; }
        emit samplerateSet(1, sampleSteps);
    }

    emit samplingStarted();
}

/// \brief Stop sampling process.
void HantekDsoControl::stopSampling() {
    sampling = false;
    emit samplingStopped();
}

const QStringList *HantekDsoControl::getSpecialTriggerSources() { return &(specialTriggerSources); }

USBDevice *HantekDsoControl::getDevice() { return device; }

const DSOsamples &HantekDsoControl::getLastSamples() { return result; }

HantekDsoControl::HantekDsoControl(USBDevice *device) : device(device),
    specification(device->getModel()->specification), controlsettings(&(specification.samplerate.single), HANTEK_CHANNELS) {
    if (device == nullptr) throw new std::runtime_error("No usb device for HantekDsoControl");

    // Transmission-ready control commands
    this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
    this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
    this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
    this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;

    // Instantiate the commands needed for all models
    command[BULK_FORCETRIGGER] = new BulkForceTrigger();
    command[BULK_STARTSAMPLING] = new BulkCaptureStart();
    command[BULK_ENABLETRIGGER] = new BulkTriggerEnabled();
    command[BULK_GETDATA] = new BulkGetData();
    command[BULK_GETCAPTURESTATE] = new BulkGetCaptureState();
    command[BULK_SETGAIN] = new BulkSetGain();

    if (specification.useControlNoBulk)
        device->setEnableBulkTransfer(false);

    // Apply special requirements by the devices model
    device->getModel()->applyRequirements(this);

    retrieveChannelLevelData();
}

HantekDsoControl::~HantekDsoControl() {
    for (int cIndex = 0; cIndex < BULK_COUNT; ++cIndex) { delete command[cIndex]; }
    for (int cIndex = 0; cIndex < CONTROLINDEX_COUNT; ++cIndex) { delete control[cIndex]; }
}

unsigned HantekDsoControl::getChannelCount() { return HANTEK_CHANNELS; }

const std::vector<unsigned> &HantekDsoControl::getAvailableRecordLengths() { //
    return controlsettings.samplerate.limits->recordLengths;
}

double HantekDsoControl::getMinSamplerate() {
    return (double)specification.samplerate.single.base / specification.samplerate.single.maxDownsampler;
}

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
        cycleTime = (int)((double)device->getPacketSize() / (isFastRate() ? 1 : HANTEK_CHANNELS) /
                          controlsettings.samplerate.current * 250);
    else
        cycleTime = (int)((double)getRecordLength() / controlsettings.samplerate.current * 250);

    // Not more often than every 10 ms though but at least once every second
    cycleTime = qBound(10, cycleTime, 1000);
}

bool HantekDsoControl::isRollMode() const {
    return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId] == UINT_MAX;
}

bool HantekDsoControl::isFastRate() const {
    return controlsettings.samplerate.limits == &specification.samplerate.multi;
}

unsigned HantekDsoControl::getRecordLength() const {
    return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId];
}

Dso::ErrorCode HantekDsoControl::retrieveChannelLevelData() {
    // Get channel level data
    int errorCode = device->controlRead(CONTROL_VALUE, (unsigned char *)&(specification.offsetLimit),
                                    sizeof(specification.offsetLimit), (int)VALUE_OFFSETLIMITS);
    if (errorCode < 0) {
        qWarning() << tr("Couldn't get channel level data from oscilloscope");
        emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
        emit communicationError();
        return Dso::ErrorCode::CONNECTION;
    }

    return Dso::ErrorCode::NONE;
}

unsigned HantekDsoControl::calculateTriggerPoint(unsigned value) {
    unsigned result = value;

    // Each set bit inverts all bits with a lower value
    for (unsigned bitValue = 1; bitValue; bitValue <<= 1)
        if (result & bitValue) result ^= bitValue - 1;

    return result;
}

std::pair<int, unsigned> HantekDsoControl::getCaptureState() const {
    int errorCode;

    if (!specification.supportsCaptureState) return std::make_pair(CAPTURE_READY, 0);

    errorCode = device->bulkCommand(command[BULK_GETCAPTURESTATE], 1);
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    BulkResponseGetCaptureState response;
    errorCode = device->bulkRead(response.data(), response.getSize());
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    return std::make_pair((int)response.getCaptureState(), response.getTriggerPoint());
}

std::vector<unsigned char> HantekDsoControl::getSamples(unsigned &previousSampleCount) const {
    if (!specification.useControlNoBulk) {
        // Request data
        int errorCode = device->bulkCommand(command[BULK_GETDATA], 1);
        if (errorCode < 0) {
            qWarning() << "Getting sample data failed: " << libUsbErrorString(errorCode);
            emit communicationError();
            return std::vector<unsigned char>();
        }
    }

    unsigned totalSampleCount = this->getSampleCount();

    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if (totalSampleCount < previousSampleCount) {
        std::swap(totalSampleCount, previousSampleCount);
    } else {
        previousSampleCount = totalSampleCount;
    }

    unsigned dataLength = (specification.sampleSize > 8) ? totalSampleCount * 2 : totalSampleCount;

    // Save raw data to temporary buffer
    std::vector<unsigned char> data(dataLength);
    int errorcode = device->bulkReadMulti(data.data(), dataLength);
    if (errorcode < 0) {
        qWarning() << "Getting sample data failed: " << libUsbErrorString(errorcode);
        return std::vector<unsigned char>();
    }
    data.resize((size_t)errorcode);

    static unsigned id = 0;
    ++id;
    timestampDebug(QString("Received packet %1").arg(id));

    return data;
}

void HantekDsoControl::convertRawDataToSamples(const std::vector<unsigned char> &rawData) {
    const size_t totalSampleCount = (specification.sampleSize > 8) ? rawData.size() / 2 : rawData.size();

    QWriteLocker locker(&result.lock);
    result.samplerate = controlsettings.samplerate.current;
    result.append = isRollMode();
    // Prepare result buffers
    result.data.resize(HANTEK_CHANNELS);
    for (int channelCounter = 0; channelCounter < HANTEK_CHANNELS; ++channelCounter)
        result.data[channelCounter].clear();

    const unsigned extraBitsSize = specification.sampleSize - 8;             // Number of extra bits
    const unsigned short extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction

    // Convert channel data
    if (isFastRate()) {
        // Fast rate mode, one channel is using all buffers
        unsigned channel = 0;
        for (; channel < HANTEK_CHANNELS; ++channel) {
            if (controlsettings.voltage[channel].used) break;
        }

        if (channel >= HANTEK_CHANNELS) return;

        // Resize sample vector
        result.data[channel].resize(totalSampleCount);

        const unsigned gainID = controlsettings.voltage[channel].gain;
        const unsigned short limit = specification.voltageLimit[channel][gainID];
        const double offset = controlsettings.voltage[channel].offsetReal;
        const double gainStep = specification.gainSteps[gainID];

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned bufferPosition = controlsettings.trigger.point * 2;
        if (specification.sampleSize > 8) {
            for (unsigned pos = 0; pos < totalSampleCount; ++pos, ++bufferPosition) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                const unsigned short low = rawData[bufferPosition];
                const unsigned extraBitsPosition = bufferPosition % HANTEK_CHANNELS;
                const unsigned shift = (8 - (HANTEK_CHANNELS - 1 - extraBitsPosition) * extraBitsSize);
                const unsigned short high =
                    ((unsigned short int)rawData[totalSampleCount + bufferPosition - extraBitsPosition] << shift) &
                    extraBitsMask;

                result.data[channel][pos] = ((double)(low + high) / limit - offset) * gainStep;
            }
        } else {
            for (unsigned pos = 0; pos < totalSampleCount; ++pos, ++bufferPosition) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                double dataBuf = (double)((int)rawData[bufferPosition]);
                result.data[channel][pos] = (dataBuf / limit - offset) * gainStep;
            }
        }
    } else {
        // Normal mode, channels are using their separate buffers
        for (unsigned channel = 0; channel < HANTEK_CHANNELS; ++channel) {
            result.data[channel].resize(totalSampleCount / HANTEK_CHANNELS);

            const unsigned gainID = controlsettings.voltage[channel].gain;
            const unsigned short limit = specification.voltageLimit[channel][gainID];
            const double offset = controlsettings.voltage[channel].offsetReal;
            const double gainStep = specification.gainSteps[gainID];

            // Convert data from the oscilloscope and write it into the sample buffer
            unsigned bufferPosition = controlsettings.trigger.point * 2;
            if (specification.sampleSize > 8) {
                // Additional most significant bits after the normal data
                unsigned extraBitsIndex = 8 - channel * 2; // Bit position offset for extra bits extraction

                for (unsigned realPosition = 0; realPosition < result.data[channel].size();
                     ++realPosition, bufferPosition += HANTEK_CHANNELS) {
                    if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                    const unsigned short low = rawData[bufferPosition + HANTEK_CHANNELS - 1 - channel];
                    const unsigned short high =
                        ((unsigned short int)rawData[totalSampleCount + bufferPosition] << extraBitsIndex) &
                        extraBitsMask;

                    result.data[channel][realPosition] = ((double)(low + high) / limit - offset) * gainStep;
                }
            } else if (device->getModel()->ID == ModelDSO6022BE::ID) {
                // if device is 6022BE, drop heading & trailing samples
                const unsigned DROP_DSO6022_HEAD = 0x410;
                const unsigned DROP_DSO6022_TAIL = 0x3F0;
                if (!isRollMode()) {
                    result.data[channel].resize(result.data[channel].size() - (DROP_DSO6022_HEAD + DROP_DSO6022_TAIL));
                    // if device is 6022BE, offset DROP_DSO6022_HEAD incrementally
                    bufferPosition += DROP_DSO6022_HEAD * 2;
                }
                bufferPosition += channel;
                for (unsigned pos = 0; pos < result.data[channel].size(); ++pos, bufferPosition += HANTEK_CHANNELS) {
                    if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;
                    double dataBuf = (double)((int)(rawData[bufferPosition] - 0x83));
                    result.data[channel][pos] = (dataBuf / limit) * gainStep;
                }
            } else {
                bufferPosition += HANTEK_CHANNELS - 1 - channel;
                for (unsigned pos = 0; pos < result.data[channel].size(); ++pos, bufferPosition += HANTEK_CHANNELS) {
                    if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;
                    double dataBuf = (double)((int)(rawData[bufferPosition]));
                    result.data[channel][pos] = (dataBuf / limit - offset) * gainStep;
                }
            }
        }
    }
}

double HantekDsoControl::getBestSamplerate(double samplerate, bool fastRate, bool maximum,
                                           unsigned *downsampler) const {
    // Abort if the input value is invalid
    if (samplerate <= 0.0) return 0.0;

    double bestSamplerate = 0.0;

    // Get samplerate specifications for this mode and model
    const ControlSamplerateLimits *limits;
    if (fastRate)
        limits = &(specification.samplerate.multi);
    else
        limits = &(specification.samplerate.single);

    // Get downsampling factor that would provide the requested rate
    double bestDownsampler = limits->base / specification.bufferDividers[controlsettings.recordLengthId] / samplerate;
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

unsigned HantekDsoControl::getSampleCount() const {
    if (isRollMode()) {
        // TODO handle libusb error
        return device->getPacketSize();
    } else {
        if (isFastRate())
            return getRecordLength();
        else
            return getRecordLength() * HANTEK_CHANNELS;
    }
}

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

            commandSetBuffer5200->setUsedPre(DTriggerPositionUsed::DTRIGGERPOSITION_ON);
            commandSetBuffer5200->setUsedPost(DTriggerPositionUsed::DTRIGGERPOSITION_ON);
            commandSetBuffer5200->setRecordLength(index);
        }

        commandPending[BULK_DSETBUFFER] = true;

        break;

    default:
        return 0;
    }

    // Check if the divider has changed and adapt samplerate limits accordingly
    bool bDividerChanged =
        specification.bufferDividers[index] != specification.bufferDividers[controlsettings.recordLengthId];

    controlsettings.recordLengthId = index;

    if (bDividerChanged) {
        this->updateSamplerateLimits();

        // Samplerate dividers changed, recalculate it
        this->restoreTargets();
    }

    return controlsettings.samplerate.limits->recordLengths[index];
}

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
        controlsettings.samplerate.current = controlsettings.samplerate.limits->base /
                                             specification.bufferDividers[controlsettings.recordLengthId] / downsampler;
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

void HantekDsoControl::restoreTargets() {
    if (controlsettings.samplerate.target.samplerateSet)
        this->setSamplerate();
    else
        this->setRecordTime();
}

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
Dso::ErrorCode HantekDsoControl::setRecordLength(unsigned index) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (!this->updateRecordLength(index)) return Dso::ErrorCode::PARAMETER;

    this->restoreTargets();
    this->setPretriggerPosition(controlsettings.trigger.position);

    emit recordLengthChanged(getRecordLength());
    return Dso::ErrorCode::NONE;
}

/// \brief Sets the samplerate of the oscilloscope.
/// \param samplerate The samplerate that should be met (S/s), 0.0 to restore
/// current samplerate.
/// \return The samplerate that has been set, 0.0 on error.
Dso::ErrorCode HantekDsoControl::setSamplerate(double samplerate) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (samplerate == 0.0) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = true;
    }

    if (!specification.isSoftwareTriggerDevice) {
        // When possible, enable fast rate if it is required to reach the requested
        // samplerate
        bool fastRate = (controlsettings.usedChannels <= 1) &&
                        (samplerate > specification.samplerate.single.max /
                                          specification.bufferDividers[controlsettings.recordLengthId]);

        // What is the nearest, at least as high samplerate the scope can provide?
        unsigned downsampler = 0;
        getBestSamplerate(samplerate, fastRate, false, &(downsampler));

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return Dso::ErrorCode::PARAMETER;
        else {
            return Dso::ErrorCode::NONE;
        }
    } else {
        unsigned sampleId;
        for (sampleId = 0; sampleId < specification.sampleSteps.size() - 1; ++sampleId)
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

        return Dso::ErrorCode::NONE;
    }
}

/// \brief Sets the time duration of one aquisition by adapting the samplerate.
/// \param duration The record time duration that should be met (s), 0.0 to
/// restore current record time.
/// \return The record time duration that has been set, 0.0 on error.
Dso::ErrorCode HantekDsoControl::setRecordTime(double duration) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

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
                        (maxSamplerate >= specification.samplerate.multi.base /
                                              specification.bufferDividers[controlsettings.recordLengthId]);

        // What is the nearest, at most as high samplerate the scope can provide?
        unsigned downsampler = 0;

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return Dso::ErrorCode::PARAMETER;
        else {
            return Dso::ErrorCode::NONE;
        }
    } else {
        // For now - we go for the 10240 size sampling - the other seems not to be
        // supported
        // Find highest samplerate using less than 10240 samples to obtain our
        // duration.
        // Better add some margin for our SW trigger
        unsigned sampleMargin = 2000;
        unsigned sampleCount = 10240;
        unsigned bestId = 0;
        unsigned sampleId;
        for (sampleId = 0; sampleId < specification.sampleSteps.size(); ++sampleId) {
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
        return Dso::ErrorCode::NONE;
    }
}

/// \brief Enables/disables filtering of the given channel.
/// \param channel The channel that should be set.
/// \param used true if the channel should be sampled.
/// \return See ::Dso::ErrorCode.
Dso::ErrorCode HantekDsoControl::setChannelUsed(unsigned channel, bool used) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ErrorCode::PARAMETER;

    // Update settings
    controlsettings.voltage[channel].used = used;
    unsigned channelCount = 0;
    for (unsigned c = 0; c < HANTEK_CHANNELS; ++c) {
        if (controlsettings.voltage[c].used) ++channelCount;
    }

    // Calculate the UsedChannels field for the command
    UsedChannels usedChannels = UsedChannels::USED_CH1;

    if (controlsettings.voltage[1].used) {
        if (controlsettings.voltage[0].used) {
            usedChannels = UsedChannels::USED_CH1CH2;
        } else {
            // DSO-2250 uses a different value for channel 2
            if (specification.command.bulk.setChannels == BULK_BSETCHANNELS)
                usedChannels = UsedChannels::BUSED_CH2;
            else
                usedChannels = UsedChannels::USED_CH2;
        }
    }

    switch (specification.command.bulk.setChannels) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger source
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])
            ->setUsedChannels((uint8_t)usedChannels);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
        break;
    }
    case BULK_BSETCHANNELS: {
        // SetChannels2250 bulk command for active channels
        static_cast<BulkSetChannels2250 *>(command[BULK_BSETCHANNELS])->setUsedChannels((uint8_t)usedChannels);
        commandPending[BULK_BSETCHANNELS] = true;

        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200s bulk command for trigger source
        static_cast<BulkSetTrigger5200 *>(command[BULK_ESETTRIGGERORSAMPLERATE])->setUsedChannels((uint8_t)usedChannels);
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

    return Dso::ErrorCode::NONE;
}

/// \brief Set the coupling for the given channel.
/// \param channel The channel that should be set.
/// \param coupling The new coupling for the channel.
/// \return See ::Dso::ErrorCode.
Dso::ErrorCode HantekDsoControl::setCoupling(unsigned channel, Dso::Coupling coupling) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ErrorCode::PARAMETER;

    // SetRelays control command for coupling relays
    if (specification.supportsCouplingRelays) {
        static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS])
            ->setCoupling(channel, coupling != Dso::COUPLING_AC);
        this->controlPending[CONTROLINDEX_SETRELAYS] = true;
    }

    return Dso::ErrorCode::NONE;
}

/// \brief Sets the gain for the given channel.
/// Get the actual gain by specification.gainSteps[gainId]
/// \param channel The channel that should be set.
/// \param gain The gain that should be met (V/div).
/// \return The gain that has been set, ::Dso::ErrorCode on error.
Dso::ErrorCode HantekDsoControl::setGain(unsigned channel, double gain) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ErrorCode::PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainId;
    for (gainId = 0; gainId < specification.gainSteps.size() - 1; ++gainId)
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

    return Dso::ErrorCode::NONE;
}

/// \brief Set the offset for the given channel.
/// Get the actual offset for the channel from controlsettings.voltage[channel].offsetReal
/// \param channel The channel that should be set.
/// \param offset The new offset value (0.0 - 1.0).
Dso::ErrorCode HantekDsoControl::setOffset(unsigned channel, double offset) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ErrorCode::PARAMETER;

    Offset& channelOffLimit = specification.offsetLimit[channel].step[controlsettings.voltage[channel].gain];
    // Calculate the offset value
    // The range is given by the calibration data (convert from big endian)
    unsigned short int minimum = ((unsigned short int)*((unsigned char *)&(channelOffLimit.start)) << 8) +
                                 *((unsigned char *)&(channelOffLimit.start) + 1);
    unsigned short int maximum = ((unsigned short int)*((unsigned char *)&(channelOffLimit.end)) << 8) +
                                 *((unsigned char *)&(channelOffLimit.end) + 1);
    unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
    double offsetReal = (double)(offsetValue - minimum) / (maximum - minimum);

    if (specification.supportsOffset) {
        static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setChannel(channel, offsetValue);
        this->controlPending[CONTROLINDEX_SETOFFSET] = true;
    }

    controlsettings.voltage[channel].offset = offset;
    controlsettings.voltage[channel].offsetReal = offsetReal;

    this->setTriggerLevel(channel, controlsettings.trigger.level[channel]);

    return Dso::ErrorCode::NONE;
}

/// \brief Set the trigger mode.
/// \return See ::Dso::ErrorCode.
Dso::ErrorCode HantekDsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (mode < Dso::TRIGGERMODE_AUTO || mode >= Dso::TRIGGERMODE_COUNT) return Dso::ErrorCode::PARAMETER;

    controlsettings.trigger.mode = mode;
    return Dso::ErrorCode::NONE;
}

/// \brief Set the trigger source.
/// \param special true for a special channel (EXT, ...) as trigger source.
/// \param id The number of the channel, that should be used as trigger.
/// \return See ::Dso::ErrorCode.
Dso::ErrorCode HantekDsoControl::setTriggerSource(bool special, unsigned id) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if ((!special && id >= HANTEK_CHANNELS) || (special && id >= HANTEK_SPECIAL_CHANNELS))
        return Dso::ErrorCode::PARAMETER;

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
        return Dso::ErrorCode::UNSUPPORTED;
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

    return Dso::ErrorCode::NONE;
}

/// \brief Set the trigger level.
/// \param channel The channel that should be set.
/// \param level The new trigger level (V).
/// \return The trigger level that has been set, ::Dso::ErrorCode on error.
Dso::ErrorCode HantekDsoControl::setTriggerLevel(unsigned channel, double level) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= HANTEK_CHANNELS) return Dso::ErrorCode::PARAMETER;

    // Calculate the trigger level value
    unsigned short minimum, maximum;
    if (specification.sampleSize > 8) {
        Offset& offsetLimit = specification.offsetLimit[channel].step[controlsettings.voltage[channel].gain];
        // The range is the same as used for the offsets for 10 bit models
        minimum = ((unsigned short)*((unsigned char *)&(offsetLimit.start)) << 8) +
                  *((unsigned char *)&(offsetLimit.start) + 1);
        maximum = ((unsigned short)*((unsigned char *)&(offsetLimit.end)) << 8) +
                  *((unsigned char *)&(offsetLimit.end) + 1);
    } else {
        // It's from 0x00 to 0xfd for the 8 bit models
        minimum = 0x00;
        maximum = 0xfd;
    }

    // Never get out of the limits
    const double offsetReal = controlsettings.voltage[channel].offsetReal;
    const double gainStep = specification.gainSteps[controlsettings.voltage[channel].gain];
    unsigned short levelValue = qBound(
        minimum, (unsigned short)(((offsetReal + level / gainStep) * (maximum - minimum) + 0.5) + minimum), maximum);

    // Check if the set channel is the trigger source
    if (!controlsettings.trigger.special && channel == controlsettings.trigger.source && specification.supportsOffset) {
        // SetOffset control command for trigger level
        static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setTrigger(levelValue);
        this->controlPending[CONTROLINDEX_SETOFFSET] = true;
    }

    /// \todo Get alternating trigger in here

    controlsettings.trigger.level[channel] = level;
    return Dso::ErrorCode::NONE;
}

/// \brief Set the trigger slope.
/// \param slope The Slope that should cause a trigger.
/// \return See ::Dso::ErrorCode.
Dso::ErrorCode HantekDsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (slope >= Dso::SLOPE_COUNT) return Dso::ErrorCode::PARAMETER;

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
        return Dso::ErrorCode::UNSUPPORTED;
    }

    controlsettings.trigger.slope = slope;
    return Dso::ErrorCode::NONE;
}

void HantekDsoControl::forceTrigger() { commandPending[BULK_FORCETRIGGER] = true; }

/// \brief Set the trigger position.
/// \param position The new trigger position (in s).
/// \return The trigger position that has been set.
Dso::ErrorCode HantekDsoControl::setPretriggerPosition(double position) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    // All trigger positions are measured in samples
    double positionSamples = position * controlsettings.samplerate.current;
    unsigned recordLength = getRecordLength();
    // Fast rate mode uses both channels
    if (controlsettings.samplerate.limits == &specification.samplerate.multi) positionSamples /= HANTEK_CHANNELS;

    switch (specification.command.bulk.setPretrigger) {
    case BULK_SETTRIGGERANDSAMPLERATE: {
        // Calculate the position value (Start point depending on record length)
        unsigned position = isRollMode() ? 0x1 : 0x7ffff - recordLength + (unsigned)positionSamples;

        // SetTriggerAndSamplerate bulk command for trigger position
        static_cast<BulkSetTriggerAndSamplerate *>(command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(position);
        commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

        break;
    }
    case BULK_FSETBUFFER: {
        // Calculate the position values (Inverse, maximum is 0x7ffff)
        unsigned positionPre = 0x7ffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0x7ffff - (unsigned)positionSamples;

        // SetBuffer2250 bulk command for trigger position
        BulkSetBuffer2250 *commandSetBuffer2250 = static_cast<BulkSetBuffer2250 *>(command[BULK_FSETBUFFER]);
        commandSetBuffer2250->setTriggerPositionPre(positionPre);
        commandSetBuffer2250->setTriggerPositionPost(positionPost);
        commandPending[BULK_FSETBUFFER] = true;

        break;
    }
    case BULK_ESETTRIGGERORSAMPLERATE: {
        // Calculate the position values (Inverse, maximum is 0xffff)
        unsigned positionPre = 0xffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0xffff - (unsigned)positionSamples;

        // SetBuffer5200 bulk command for trigger position
        BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(command[BULK_DSETBUFFER]);
        commandSetBuffer5200->setTriggerPositionPre((unsigned short)positionPre);
        commandSetBuffer5200->setTriggerPositionPost((unsigned short)positionPost);
        commandPending[BULK_DSETBUFFER] = true;

        break;
    }
    default:
        return Dso::ErrorCode::UNSUPPORTED;
    }

    controlsettings.trigger.position = position;
    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::stringCommand(const QString &commandString) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    QStringList commandParts = commandString.split(' ', QString::SkipEmptyParts);

    if (commandParts.count() < 1) return Dso::ErrorCode::PARAMETER;

    if (commandParts[0] != "send") return Dso::ErrorCode::UNSUPPORTED;

    if (commandParts.count() < 2) return Dso::ErrorCode::PARAMETER;

    if (commandParts[1] == "bulk") {
        QString data = commandString.section(' ', 2, -1, QString::SectionSkipEmpty);
        unsigned char commandCode = 0;

        // Read command code (First byte)
        hexParse(commandParts[2], &commandCode, 1);
        if (commandCode > BULK_COUNT) return Dso::ErrorCode::UNSUPPORTED;

        // Update bulk command and mark as pending
        hexParse(data, command[commandCode]->data(), command[commandCode]->getSize());
        commandPending[commandCode] = true;
        return Dso::ErrorCode::NONE;
    } else if (commandParts[1] == "control") {
        unsigned char controlCode = 0;

        // Read command code (First byte)
        hexParse(commandParts[2], &controlCode, 1);
        int cIndex;
        for (cIndex = 0; cIndex < CONTROLINDEX_COUNT; ++cIndex) {
            if (this->controlCode[cIndex] == controlCode) break;
        }
        if (cIndex >= CONTROLINDEX_COUNT) return Dso::ErrorCode::UNSUPPORTED;

        QString data = commandString.section(' ', 3, -1, QString::SectionSkipEmpty);

        // Update control command and mark as pending
        hexParse(data, this->control[cIndex]->data(), this->control[cIndex]->getSize());
        this->controlPending[cIndex] = true;
        return Dso::ErrorCode::NONE;
    } else
        return Dso::ErrorCode::UNSUPPORTED;
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
            emit communicationError();
            return;
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
        case RollState::STARTSAMPLING:
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

        case RollState::ENABLETRIGGER:
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

        case RollState::FORCETRIGGER:
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

        case RollState::GETDATA: {
            std::vector<unsigned char> rawData = this->getSamples(previousSampleCount);
            if (this->_samplingStarted) {
                convertRawDataToSamples(rawData);
                emit samplesAvailable();
            }
        }

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
        if (toNextState) this->rollState = (RollState)(((int)rollState + 1) % (int)RollState::_COUNT);
    } else {
        // Standard mode
        this->rollState = RollState::STARTSAMPLING;

        const int lastCaptureState = this->captureState;
        unsigned triggerPoint;
        std::tie(captureState, triggerPoint) = this->getCaptureState();
        controlsettings.trigger.point = calculateTriggerPoint(triggerPoint);
        if (this->captureState < 0) {
            qWarning() << tr("Getting capture state failed: %1").arg(libUsbErrorString(this->captureState));
            emit statusMessage(tr("Getting capture state failed: %1").arg(libUsbErrorString(this->captureState)), 0);
        } else if (this->captureState != lastCaptureState)
            timestampDebug(QString("Capture state changed to %1").arg(this->captureState));

        switch (this->captureState) {
        case CAPTURE_READY:
        case CAPTURE_READY2250:
        case CAPTURE_READY5200: {
            std::vector<unsigned char> rawData = this->getSamples(previousSampleCount);
            if (this->_samplingStarted) {
                convertRawDataToSamples(rawData);
                emit samplesAvailable();
            }
        }

            // Check if we're in single trigger mode
            if (controlsettings.trigger.mode == Dso::TRIGGERMODE_SINGLE && this->_samplingStarted) this->stopSampling();

            // Sampling completed, restart it when necessary
            this->_samplingStarted = false;

            // Start next capture if necessary by leaving out the break statement
            if (!this->sampling) break; else [[fallthrough]];

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
            this->startCycle = int(controlsettings.trigger.position * 1000.0 / cycleTime + 1.0);
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
