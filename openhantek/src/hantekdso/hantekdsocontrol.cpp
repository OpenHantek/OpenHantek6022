// SPDX-License-Identifier: GPL-2.0+

#include <assert.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

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

using namespace Hantek;
using namespace Dso;

/// \brief Start sampling process.
void HantekDsoControl::startSampling() {
    sampling = true;

    // Emit signals for initial settings
    emit availableRecordLengthsChanged(controlsettings.samplerate.limits->recordLengths);
    updateSamplerateLimits();
    emit recordLengthChanged(getRecordLength());
    if (!isRollMode()) emit recordTimeChanged((double)getRecordLength() / controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    if (specification.isFixedSamplerateDevice) {
        // Convert to GUI presentable values (1e5 -> 1.0, 48e6 -> 480.0 etc)
        QList<double> sampleSteps;
        for (auto &v : specification.fixedSampleRates) { sampleSteps << v.samplerate / 1e5; }
        emit samplerateSet(1, sampleSteps);
    }

    emit samplingStarted();
}

/// \brief Stop sampling process.
void HantekDsoControl::stopSampling() {
    sampling = false;
    emit samplingStopped();
}

const USBDevice *HantekDsoControl::getDevice() const { return device; }

const DSOsamples &HantekDsoControl::getLastSamples() { return result; }

HantekDsoControl::HantekDsoControl(USBDevice *device)
    : device(device), specification(device->getModel()->specification),
      controlsettings(&(specification.samplerate.single), specification.channels) {
    if (device == nullptr) throw new std::runtime_error("No usb device for HantekDsoControl");

    qRegisterMetaType<DSOsamples *>();

    if (specification.fixedUSBinLength)
    device->overwriteInPacketLength(specification.fixedUSBinLength);

    // Apply special requirements by the devices model
    device->getModel()->applyRequirements(this);

    retrieveChannelLevelData();
}

HantekDsoControl::~HantekDsoControl() {
    while (firstBulkCommand) {
        BulkCommand *t = firstBulkCommand->next;
        delete firstBulkCommand;
        firstBulkCommand = t;
    }
    while (firstControlCommand) {
        ControlCommand *t = firstControlCommand->next;
        delete firstControlCommand;
        firstControlCommand = t;
    }
}

int HantekDsoControl::bulkCommand(const DataArray<unsigned char> *command, int attempts) const {
    if (specification.useControlNoBulk) return LIBUSB_SUCCESS;

    // Send BeginCommand control command
    int errorCode = device->controlWrite(&specification.beginCommandControl);
    if (errorCode < 0) return errorCode;

    // Send bulk command
    return device->bulkWrite(command->data(), command->getSize(), attempts);
}

unsigned HantekDsoControl::getChannelCount() { return specification.channels; }

const ControlSettings *HantekDsoControl::getDeviceSettings() const { return &controlsettings; }

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
        cycleTime = (int)((double)getPacketSize() / (isFastRate() ? 1 : specification.channels) /
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
    int errorCode = device->controlRead(&specification.cmdGetLimits);
    if (errorCode < 0) {
        qWarning() << tr("Couldn't get channel level data from oscilloscope");
        emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
        emit communicationError();
        return Dso::ErrorCode::CONNECTION;
    }

    memcpy(controlsettings.offsetLimit, specification.cmdGetLimits.offsetLimit,
           sizeof(OffsetsPerGainStep)*specification.channels);

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

    errorCode = bulkCommand(getCommand(BulkCode::GETCAPTURESTATE), 1);
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    BulkResponseGetCaptureState response;
    errorCode = device->bulkRead(&response);
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    return std::make_pair((int)response.getCaptureState(), response.getTriggerPoint());
}

std::vector<unsigned char> HantekDsoControl::getSamples(unsigned &previousSampleCount) const {
    int errorCode;
    if (!specification.useControlNoBulk) {
        // Request data
        errorCode = bulkCommand(getCommand(BulkCode::GETDATA), 1);
    } else {
        errorCode = device->controlWrite(getCommand(ControlCode::CONTROL_ACQUIIRE_HARD_DATA));
    }
    if (errorCode < 0) {
        qWarning() << "Getting sample data failed: " << libUsbErrorString(errorCode);
        emit communicationError();
        return std::vector<unsigned char>();
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
    result.data.resize(specification.channels);
    for (ChannelID channelCounter = 0; channelCounter < specification.channels; ++channelCounter)
        result.data[channelCounter].clear();

    const unsigned extraBitsSize = specification.sampleSize - 8;             // Number of extra bits
    const unsigned short extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction

    // Convert channel data
    if (isFastRate()) {
        // Fast rate mode, one channel is using all buffers
        ChannelID channel = 0;
        for (; channel < specification.channels; ++channel) {
            if (controlsettings.voltage[channel].used) break;
        }

        if (channel >= specification.channels) return;

        // Resize sample vector
        result.data[channel].resize(totalSampleCount);

        const unsigned gainID = controlsettings.voltage[channel].gain;
        const unsigned short limit = specification.voltageLimit[channel][gainID];
        const double offset = controlsettings.voltage[channel].offsetReal;
        const double gainStep = specification.gain[gainID].gainSteps;

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned bufferPosition = controlsettings.trigger.point * 2;
        if (specification.sampleSize > 8) {
            for (unsigned pos = 0; pos < totalSampleCount; ++pos, ++bufferPosition) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                const unsigned short low = rawData[bufferPosition];
                const unsigned extraBitsPosition = bufferPosition % specification.channels;
                const unsigned shift = (8 - (specification.channels - 1 - extraBitsPosition) * extraBitsSize);
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
        for (ChannelID channel = 0; channel < specification.channels; ++channel) {
            result.data[channel].resize(totalSampleCount / specification.channels);

            const unsigned gainID = controlsettings.voltage[channel].gain;
            const unsigned short limit = specification.voltageLimit[channel][gainID];
            const double offset = controlsettings.voltage[channel].offsetReal;
            const double gainStep = specification.gain[gainID].gainSteps;
            int shiftDataBuf = 0;

            // Convert data from the oscilloscope and write it into the sample buffer
            unsigned bufferPosition = controlsettings.trigger.point * 2;
            if (specification.sampleSize > 8) {
                // Additional most significant bits after the normal data
                unsigned extraBitsIndex = 8 - channel * 2; // Bit position offset for extra bits extraction

                for (unsigned realPosition = 0; realPosition < result.data[channel].size();
                     ++realPosition, bufferPosition += specification.channels) {
                    if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                    const unsigned short low = rawData[bufferPosition + specification.channels - 1 - channel];
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
                shiftDataBuf = 0x83;
            } else {
                bufferPosition += specification.channels - 1 - channel;
            }
            for (unsigned pos = 0; pos < result.data[channel].size(); ++pos, bufferPosition += specification.channels) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;
                double dataBuf = (double)((int)(rawData[bufferPosition] - shiftDataBuf));
                result.data[channel][pos] = (dataBuf / limit - offset) * gainStep;
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
        switch (specification.cmdSetSamplerate) {
        case BulkCode::SETTRIGGERANDSAMPLERATE:
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

        case BulkCode::CSETTRIGGERORSAMPLERATE:
            // DSO-5200 may not supports all downsampling factors, requires testing
            if (maximum) {
                bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
            } else {
                bestDownsampler = floor(bestDownsampler); // Round down to next integer value
            }
            break;

        case BulkCode::ESETTRIGGERORSAMPLERATE:
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
        return getPacketSize();
    } else {
        if (isFastRate())
            return getRecordLength();
        else
            return getRecordLength() * specification.channels;
    }
}

unsigned HantekDsoControl::updateRecordLength(RecordLengthID index) {
    if (index >= controlsettings.samplerate.limits->recordLengths.size()) return 0;

    switch (specification.cmdSetRecordLength) {
    case BulkCode::SETTRIGGERANDSAMPLERATE:
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setRecordLength(index);
        break;

    case BulkCode::DSETBUFFER:
        if (specification.cmdSetPretrigger == BulkCode::FSETBUFFER) {
            modifyCommand<BulkSetRecordLength2250>(BulkCode::DSETBUFFER)->setRecordLength(index);
        } else {
            // SetBuffer5200 bulk command for record length
            BulkSetBuffer5200 *commandSetBuffer5200 = modifyCommand<BulkSetBuffer5200>(BulkCode::DSETBUFFER);

            commandSetBuffer5200->setUsedPre(DTriggerPositionUsed::ON);
            commandSetBuffer5200->setUsedPost(DTriggerPositionUsed::ON);
            commandSetBuffer5200->setRecordLength(index);
        }

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
    ControlSamplerateLimits *limits = fastRate ? &specification.samplerate.multi : &specification.samplerate.single;

    // Set the calculated samplerate
    switch (specification.cmdSetSamplerate) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
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
            modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE);

        // Store if samplerate ID or downsampling factor is used
        commandSetTriggerAndSamplerate->setDownsamplingMode(downsampling);
        // Store samplerate ID
        commandSetTriggerAndSamplerate->setSamplerateId(samplerateId);
        // Store downsampling factor
        commandSetTriggerAndSamplerate->setDownsampler(downsamplerValue);
        // Set fast rate when used
        commandSetTriggerAndSamplerate->setFastRate(false /*fastRate*/);

        break;
    }
    case BulkCode::CSETTRIGGERORSAMPLERATE: {
        // Split the resulting divider into the values understood by the device
        // The fast value is kept at 4 (or 3) for slow sample rates
        long int valueSlow = qMax(((long int)downsampler - 3) / 2, (long int)0);
        unsigned char valueFast = downsampler - valueSlow * 2;

        // Pointers to needed commands
        BulkSetSamplerate5200 *commandSetSamplerate5200 =
            modifyCommand<BulkSetSamplerate5200>(BulkCode::CSETTRIGGERORSAMPLERATE);
        BulkSetTrigger5200 *commandSetTrigger5200 =
            modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE);

        // Store samplerate fast value
        commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
        // Store samplerate slow value (two's complement)
        commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
        // Set fast rate when used
        commandSetTrigger5200->setFastRate(fastRate);

        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // Pointers to needed commands
        BulkSetSamplerate2250 *commandSetSamplerate2250 =
            modifyCommand<BulkSetSamplerate2250>(BulkCode::ESETTRIGGERORSAMPLERATE);

        bool downsampling = downsampler >= 1;
        // Store downsampler state value
        commandSetSamplerate2250->setDownsampling(downsampling);
        // Store samplerate value
        commandSetSamplerate2250->setSamplerate(downsampler > 1 ? 0x10001 - downsampler : 0);
        // Set fast rate when used
        commandSetSamplerate2250->setFastRate(fastRate);

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
    if (controlsettings.samplerate.target.samplerateSet == ControlSettingsSamplerateTarget::Samplerrate)
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

Dso::ErrorCode HantekDsoControl::setRecordLength(unsigned index) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (!updateRecordLength(index)) return Dso::ErrorCode::PARAMETER;

    restoreTargets();
    setPretriggerPosition(controlsettings.trigger.position);

    emit recordLengthChanged(getRecordLength());
    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setSamplerate(double samplerate) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (samplerate == 0.0) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Samplerrate;
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
        for (sampleId = 0; sampleId < specification.fixedSampleRates.size() - 1; ++sampleId)
            if (specification.fixedSampleRates[sampleId].samplerate == samplerate) break;
        modifyCommand<ControlSetTimeDIV>(ControlCode::CONTROL_SETTIMEDIV)
            ->setDiv(specification.fixedSampleRates[sampleId].id);
        controlsettings.samplerate.current = samplerate;

        // Check for Roll mode
        if (!isRollMode())
            emit recordTimeChanged((double)(getRecordLength() - controlsettings.swSampleMargin) /
                                   controlsettings.samplerate.current);
        emit samplerateChanged(controlsettings.samplerate.current);

        return Dso::ErrorCode::NONE;
    }
}

Dso::ErrorCode HantekDsoControl::setRecordTime(double duration) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (duration == 0.0) {
        duration = controlsettings.samplerate.target.duration;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Duration;
    }

    if (!specification.isFixedSamplerateDevice) {
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
        // For now - we go for the 10240 size sampling - the other seems not to be supported
        // Find highest samplerate using less than 10240 samples to obtain our duration.
        unsigned sampleCount = 10240;
        if (specification.isSoftwareTriggerDevice) sampleCount -= controlsettings.swSampleMargin;
        unsigned sampleId;
        for (sampleId = 0; sampleId < specification.fixedSampleRates.size(); ++sampleId) {
            if (specification.fixedSampleRates[sampleId].samplerate * duration < sampleCount)
                break;
        }
        // Usable sample value
        modifyCommand<ControlSetTimeDIV>(ControlCode::CONTROL_SETTIMEDIV)
            ->setDiv(specification.fixedSampleRates[sampleId].id);
        controlsettings.samplerate.current = specification.fixedSampleRates[sampleId].samplerate;

        emit samplerateChanged(controlsettings.samplerate.current);
        return Dso::ErrorCode::NONE;
    }
}

Dso::ErrorCode HantekDsoControl::setChannelUsed(ChannelID channel, bool used) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification.channels) return Dso::ErrorCode::PARAMETER;

    // Update settings
    controlsettings.voltage[channel].used = used;
    ChannelID channelCount = 0;
    for (unsigned c = 0; c < specification.channels; ++c) {
        if (controlsettings.voltage[c].used) ++channelCount;
    }

    // Calculate the UsedChannels field for the command
    UsedChannels usedChannels = UsedChannels::USED_CH1;

    if (controlsettings.voltage[1].used) {
        if (controlsettings.voltage[0].used) {
            usedChannels = UsedChannels::USED_CH1CH2;
        } else {
            // DSO-2250 uses a different value for channel 2
            if (specification.cmdSetChannels == BulkCode::BSETCHANNELS)
                usedChannels = UsedChannels::BUSED_CH2;
            else
                usedChannels = UsedChannels::USED_CH2;
        }
    }

    switch (specification.cmdSetChannels) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger source
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)
            ->setUsedChannels((uint8_t)usedChannels);
        break;
    }
    case BulkCode::BSETCHANNELS: {
        // SetChannels2250 bulk command for active channels
        modifyCommand<BulkSetChannels2250>(BulkCode::BSETCHANNELS)->setUsedChannels((uint8_t)usedChannels);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200s bulk command for trigger source
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setUsedChannels((uint8_t)usedChannels);
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

Dso::ErrorCode HantekDsoControl::setCoupling(ChannelID channel, Dso::Coupling coupling) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification.channels) return Dso::ErrorCode::PARAMETER;

    // SetRelays control command for coupling relays
    if (specification.supportsCouplingRelays) {
        modifyCommand<ControlSetRelays>(ControlCode::CONTROL_SETRELAYS)
            ->setCoupling(channel, coupling != Dso::Coupling::AC);
    }

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setGain(ChannelID channel, double gain) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification.channels) return Dso::ErrorCode::PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainID;
    for (gainID = 0; gainID < specification.gain.size() - 1; ++gainID)
        if (specification.gain[gainID].gainSteps >= gain) break;

    if (specification.useControlNoBulk) {
        if (channel == 0) {
            modifyCommand<ControlSetVoltDIV_CH1>(ControlCode::CONTROL_SETVOLTDIV_CH1)
                ->setDiv(specification.gain[gainID].gainIndex);
        } else if (channel == 1) {
            modifyCommand<ControlSetVoltDIV_CH2>(ControlCode::CONTROL_SETVOLTDIV_CH2)
                ->setDiv(specification.gain[gainID].gainIndex);
        } else
            qDebug("%s: Unsuported channel: %i\n", __func__, channel);
    } else {
        // SetGain bulk command for gain
        modifyCommand<BulkSetGain>(BulkCode::SETGAIN)->setGain(channel, specification.gain[gainID].gainIndex);

        // SetRelays control command for gain relays
        ControlSetRelays *controlSetRelays = modifyCommand<ControlSetRelays>(ControlCode::CONTROL_SETRELAYS);
        controlSetRelays->setBelow1V(channel, gainID < 3);
        controlSetRelays->setBelow100mV(channel, gainID < 6);
    }

    controlsettings.voltage[channel].gain = gainID;

    this->setOffset(channel, controlsettings.voltage[channel].offset);

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setOffset(ChannelID channel, const double offset) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification.channels) return Dso::ErrorCode::PARAMETER;

    if (specification.supportsOffset) {
        Offset &channelOffLimit = controlsettings.offsetLimit[channel].step[controlsettings.voltage[channel].gain];
        // Calculate the offset value
        // The range is given by the calibration data (convert from big endian)
        unsigned short int minimum = ((unsigned short int)*((unsigned char *)&(channelOffLimit.start)) << 8) +
                                     *((unsigned char *)&(channelOffLimit.start) + 1);
        unsigned short int maximum = ((unsigned short int)*((unsigned char *)&(channelOffLimit.end)) << 8) +
                                     *((unsigned char *)&(channelOffLimit.end) + 1);
        unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
        double offsetReal = (double)(offsetValue - minimum) / (maximum - minimum);

        modifyCommand<ControlSetOffset>(ControlCode::CONTROL_SETOFFSET)->setChannel(channel, offsetValue);
        controlsettings.voltage[channel].offsetReal = offsetReal;
    }

    controlsettings.voltage[channel].offset = offset;

    this->setTriggerLevel(channel, controlsettings.trigger.level[channel]);

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    controlsettings.trigger.mode = mode;
    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setTriggerSource(bool special, unsigned id) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (specification.isSoftwareTriggerDevice) return Dso::ErrorCode::UNSUPPORTED;

    if (!special && id >= specification.channels) return Dso::ErrorCode::PARAMETER;

    if (special && id >= specification.specialTriggerChannels.size()) return Dso::ErrorCode::PARAMETER;

    int hardwareID = special ? specification.specialTriggerChannels[id].hardwareID : (int)id;

    switch (specification.cmdSetTrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE:
        // SetTriggerAndSamplerate bulk command for trigger source
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setTriggerSource(1 - hardwareID);
        break;

    case BulkCode::CSETTRIGGERORSAMPLERATE:
        // SetTrigger2250 bulk command for trigger source
        modifyCommand<BulkSetTrigger2250>(BulkCode::CSETTRIGGERORSAMPLERATE)->setTriggerSource(2 + hardwareID);
        break;

    case BulkCode::ESETTRIGGERORSAMPLERATE:
        // SetTrigger5200 bulk command for trigger source
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setTriggerSource(1 - hardwareID);
        break;

    default:
        return Dso::ErrorCode::UNSUPPORTED;
    }

    // SetRelays control command for external trigger relay
    modifyCommand<ControlSetRelays>(ControlCode::CONTROL_SETRELAYS)->setTrigger(special);

    controlsettings.trigger.special = special;
    controlsettings.trigger.source = id;

    // Apply trigger level of the new source
    if (special) {
        // SetOffset control command for changed trigger level
        modifyCommand<ControlSetOffset>(ControlCode::CONTROL_SETOFFSET)->setTrigger(0x7f);
    } else
        this->setTriggerLevel(id, controlsettings.trigger.level[id]);

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setTriggerLevel(ChannelID channel, double level) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification.channels) return Dso::ErrorCode::PARAMETER;

    controlsettings.trigger.level[channel] = level;
    if (!specification.supportsOffset) return Dso::ErrorCode::UNSUPPORTED;

    // Calculate the trigger level value
    unsigned short minimum, maximum;
    if (specification.sampleSize > 8) {
        Offset &offsetLimit = controlsettings.offsetLimit[channel].step[controlsettings.voltage[channel].gain];
        // The range is the same as used for the offsets for 10 bit models
        minimum = ((unsigned short)*((unsigned char *)&(offsetLimit.start)) << 8) +
                  *((unsigned char *)&(offsetLimit.start) + 1);
        maximum =
            ((unsigned short)*((unsigned char *)&(offsetLimit.end)) << 8) + *((unsigned char *)&(offsetLimit.end) + 1);
    } else {
        // It's from 0x00 to 0xfd for the 8 bit models
        minimum = 0x00;
        maximum = 0xfd;
    }

    // Never get out of the limits
    const unsigned gainID = controlsettings.voltage[channel].gain;
    const double offsetReal = controlsettings.voltage[channel].offsetReal;
    const double gainStep = specification.gain[gainID].gainSteps;
    const unsigned short levelValue = qBound(
        minimum, (unsigned short)(((offsetReal + level / gainStep) * (maximum - minimum) + 0.5) + minimum), maximum);

    // Check if the set channel is the trigger source
    if (!controlsettings.trigger.special && channel == controlsettings.trigger.source) {
        // SetOffset control command for trigger level
        modifyCommand<ControlSetOffset>(ControlCode::CONTROL_SETOFFSET)->setTrigger(levelValue);
    }

    /// \todo Get alternating trigger in here

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode HantekDsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    switch (specification.cmdSetTrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger slope
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    case BulkCode::CSETTRIGGERORSAMPLERATE: {
        // SetTrigger2250 bulk command for trigger slope
        modifyCommand<BulkSetTrigger2250>(BulkCode::CSETTRIGGERORSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200 bulk command for trigger slope
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    default:
        return Dso::ErrorCode::UNSUPPORTED;
    }

    controlsettings.trigger.slope = slope;
    return Dso::ErrorCode::NONE;
}

void HantekDsoControl::forceTrigger() { modifyCommand<BulkCommand>(BulkCode::FORCETRIGGER); }

Dso::ErrorCode HantekDsoControl::setPretriggerPosition(double position) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    // All trigger positions are measured in samples
    double positionSamples = position * controlsettings.samplerate.current;
    unsigned recordLength = getRecordLength();
    // Fast rate mode uses both channels
    if (isFastRate()) positionSamples /= specification.channels;

    switch (specification.cmdSetPretrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // Calculate the position value (Start point depending on record length)
        unsigned triggerPosition = isRollMode() ? 0x1 : 0x7ffff - recordLength + (unsigned)positionSamples;

        // SetTriggerAndSamplerate bulk command for trigger position
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)
            ->setTriggerPosition(triggerPosition);
        break;
    }
    case BulkCode::FSETBUFFER: {
        // Calculate the position values (Inverse, maximum is 0x7ffff)
        unsigned positionPre = 0x7ffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0x7ffff - (unsigned)positionSamples;

        // SetBuffer2250 bulk command for trigger position
        BulkSetBuffer2250 *commandSetBuffer2250 = modifyCommand<BulkSetBuffer2250>(BulkCode::FSETBUFFER);
        commandSetBuffer2250->setTriggerPositionPre(positionPre);
        commandSetBuffer2250->setTriggerPositionPost(positionPost);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // Calculate the position values (Inverse, maximum is 0xffff)
        unsigned positionPre = 0xffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0xffff - (unsigned)positionSamples;

        // SetBuffer5200 bulk command for trigger position
        BulkSetBuffer5200 *commandSetBuffer5200 = modifyCommand<BulkSetBuffer5200>(BulkCode::DSETBUFFER);
        commandSetBuffer5200->setTriggerPositionPre((unsigned short)positionPre);
        commandSetBuffer5200->setTriggerPositionPost((unsigned short)positionPost);
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

    uint8_t codeIndex = 0;
    hexParse(commandParts[2], &codeIndex, 1);
    QString data = commandString.section(' ', 2, -1, QString::SectionSkipEmpty);

    if (commandParts[1] == "bulk") {
        if (!command[codeIndex]) return Dso::ErrorCode::UNSUPPORTED;

        BulkCommand *c = modifyCommand<BulkCommand>((BulkCode)codeIndex);
        hexParse(data, c->data(), c->getSize());
        return Dso::ErrorCode::NONE;
    } else if (commandParts[1] == "control") {
        if (!control[codeIndex]) return Dso::ErrorCode::UNSUPPORTED;

        ControlCommand *c = modifyCommand<ControlCommand>((ControlCode)codeIndex);
        hexParse(data, c->data(), c->getSize());
        return Dso::ErrorCode::NONE;
    } else
        return Dso::ErrorCode::UNSUPPORTED;
}

void HantekDsoControl::addCommand(BulkCommand *newCommand, bool pending) {
    newCommand->pending = pending;
    command[(uint8_t)newCommand->code] = newCommand;
    newCommand->next = firstBulkCommand;
    firstBulkCommand = newCommand;
}

const BulkCommand *HantekDsoControl::getCommand(BulkCode code) const { return command[(uint8_t)code]; }

void HantekDsoControl::addCommand(ControlCommand *newCommand, bool pending) {
    newCommand->pending = pending;
    control[newCommand->code] = newCommand;
    newCommand->next = firstControlCommand;
    firstControlCommand = newCommand;
}

const ControlCommand *HantekDsoControl::getCommand(ControlCode code) const { return control[(uint8_t)code]; }

void HantekDsoControl::run() {
    int errorCode = 0;

    // Send all pending bulk commands
    BulkCommand *command = firstBulkCommand;
    while (command) {
        if (command->pending) {
            timestampDebug(
                QString("Sending bulk command:%1").arg(hexDump(command->data(), command->getSize())));

            errorCode = bulkCommand(command);
            if (errorCode < 0) {
                qWarning() << "Sending bulk command failed: " << libUsbErrorString(errorCode);
                emit communicationError();
                return;
            } else
                command->pending = false;
        }
        command = command->next;
    }

    // Send all pending control commands
    ControlCommand *controlCommand = firstControlCommand;
    while (controlCommand) {
        if (controlCommand->pending) {
            timestampDebug(QString("Sending control command %1:%2")
                               .arg(QString::number(controlCommand->code, 16),
                                    hexDump(controlCommand->data(), controlCommand->getSize())));

            errorCode = device->controlWrite(controlCommand);
            if (errorCode < 0) {
                qWarning("Sending control command %2x failed: %s", (uint8_t)controlCommand->code,
                         libUsbErrorString(errorCode).toLocal8Bit().data());

                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit communicationError();
                    return;
                }
            } else
                controlCommand->pending = false;
        }
        controlCommand = controlCommand->next;
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
            expectedSampleCount = this->getSampleCount();

            errorCode = bulkCommand(getCommand(BulkCode::STARTSAMPLING));
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
            errorCode = bulkCommand(getCommand(BulkCode::ENABLETRIGGER));
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
            errorCode = bulkCommand(getCommand(BulkCode::FORCETRIGGER));
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
            std::vector<unsigned char> rawData = this->getSamples(expectedSampleCount);
            if (this->_samplingStarted) {
                convertRawDataToSamples(rawData);
                emit samplesAvailable(&result);
            }
        }

            // Check if we're in single trigger mode
            if (controlsettings.trigger.mode == Dso::TriggerMode::SINGLE && this->_samplingStarted)
                this->stopSampling();

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
            std::vector<unsigned char> rawData = this->getSamples(expectedSampleCount);
            if (this->_samplingStarted) {
                convertRawDataToSamples(rawData);
                emit samplesAvailable(&result);
            }
        }

            // Check if we're in single trigger mode
            if (controlsettings.trigger.mode == Dso::TriggerMode::SINGLE && this->_samplingStarted)
                this->stopSampling();

            // Sampling completed, restart it when necessary
            this->_samplingStarted = false;

            // Start next capture if necessary by leaving out the break statement

            if (!this->sampling) break;
#if __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]];
#elif __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]];
#else
#define FALLTHROUGH
#endif
            else {
                FALLTHROUGH
            }
        case CAPTURE_WAITING:
            // Sampling hasn't started, update the expected sample count
            expectedSampleCount = this->getSampleCount();

            if (_samplingStarted && lastTriggerMode == controlsettings.trigger.mode) {
                ++this->cycleCounter;

                if (this->cycleCounter == this->startCycle && !isRollMode()) {
                    // Buffer refilled completely since start of sampling, enable the
                    // trigger now
                    errorCode = bulkCommand(getCommand(BulkCode::ENABLETRIGGER));
                    if (errorCode < 0) {
                        if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                            emit communicationError();
                            return;
                        }
                        break;
                    }

                    timestampDebug("Enabling trigger");
                } else if (cycleCounter >= 8 + this->startCycle &&
                           controlsettings.trigger.mode == Dso::TriggerMode::WAIT_FORCE) {
                    // Force triggering
                    errorCode = bulkCommand(getCommand(BulkCode::FORCETRIGGER));
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
            errorCode = bulkCommand(getCommand(BulkCode::STARTSAMPLING));
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


int HantekDsoControl::getConnectionSpeed() const {
    int errorCode;
    ControlGetSpeed response;
    errorCode = device->controlRead(&response);
    if (errorCode < 0) return errorCode;

    return response.getSpeed();
}

int HantekDsoControl::getPacketSize() const {
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
