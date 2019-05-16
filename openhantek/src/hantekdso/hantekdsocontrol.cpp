// SPDX-License-Identifier: GPL-2.0+

// #define DEBUG

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

#include <stdio.h>

#include "scopesettings.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "models/modelDSO6022.h"
#include "usb/usbdevice.h"

using namespace Hantek;
using namespace Dso;

/// \brief Start sampling process.
void HantekDsoControl::enableSampling(bool enabled) {
    sampling = enabled;

    // Emit signals for initial settings
    //    emit availableRecordLengthsChanged(controlsettings.samplerate.limits->recordLengths);
    updateSamplerateLimits();
    //    emit recordLengthChanged(getRecordLength());
    //    if (!isRollMode()) emit recordTimeChanged((double)getRecordLength() / controlsettings.samplerate.current);
    //    emit samplerateChanged(controlsettings.samplerate.current);

    emit samplingStatusChanged(enabled);
}

const USBDevice *HantekDsoControl::getDevice() const { return device; }

const DSOsamples &HantekDsoControl::getLastSamples() { return result; }

// HORO: Hantek 6022 hack
#define is6022BE (getDevice()->getModel()->ID == ModelDSO6022BE::ID)
#define is6022BL (getDevice()->getModel()->ID == ModelDSO6022BL::ID)
#define is6022 ( is6022BE || is6022BL )
// 6022BE & 6022BL firmware (custom) supports command E4 -> set channel num
// fast mode: sample only CH1 and transmit 8bit / sample instead of CH1&CH2 = 16bit / sample
#define isFast6022 ( is6022 && controlsettings.voltage[0].used && !controlsettings.voltage[1].used )

static bool channelUsedChanged = false;

HantekDsoControl::HantekDsoControl(USBDevice *device)
    : device(device), specification(device->getModel()->spec()),
      controlsettings(&(specification->samplerate.single), specification->channels) {
    if (device == nullptr) throw new std::runtime_error("No usb device for HantekDsoControl");

    qRegisterMetaType<DSOsamples *>();

    if (specification->fixedUSBinLength) device->overwriteInPacketLength(specification->fixedUSBinLength);

    // Apply special requirements by the devices model
    device->getModel()->applyRequirements(this);

    retrieveChannelLevelData();
}

HantekDsoControl::~HantekDsoControl() {
    while (firstControlCommand) {
        ControlCommand *t = firstControlCommand->next;
        delete firstControlCommand;
        firstControlCommand = t;
    }
}

#if 0
int HantekDsoControl::bulkCommand(const std::vector<unsigned char> *command, int attempts) const {
    if (specification->useControlNoBulk) return LIBUSB_SUCCESS;

    // Send BeginCommand control command
    int errorCode = device->controlWrite(&controlsettings.beginCommandControl);
    if (errorCode < 0) return errorCode;

    // Send bulk command
    return device->bulkWrite(command->data(), command->size(), attempts);
}
#endif

unsigned HantekDsoControl::getChannelCount() const { return specification->channels; }

const ControlSettings *HantekDsoControl::getDeviceSettings() const { return &controlsettings; }

const std::vector<unsigned> &HantekDsoControl::getAvailableRecordLengths() const { //
    return controlsettings.samplerate.limits->recordLengths;
}

double HantekDsoControl::getMinSamplerate() const {
    return (double)specification->samplerate.single.base / specification->samplerate.single.maxDownsampler;
}

double HantekDsoControl::getMaxSamplerate() const {
    // printf( "usedChannels %d\n", controlsettings.usedChannels );
    if (controlsettings.usedChannels <= 1) {
        return specification->samplerate.multi.max;
    } else {
        return specification->samplerate.single.max;
    }
}

bool HantekDsoControl::isSampling() const { return sampling; }

/// \brief Updates the interval of the periodic thread timer.
void HantekDsoControl::updateInterval() {
    // Check the current oscilloscope state everytime 25% of the time the buffer
    // should be refilled
    if (isRollMode())
        cycleTime = (int)((double)getPacketSize() / (isFastRate() ? 1 : specification->channels) /
                          controlsettings.samplerate.current * 250);
    else
        cycleTime = (int)((double)getRecordLength() / controlsettings.samplerate.current * 250);

    // Not more often than every 100 ms though but at least once every second
    cycleTime = qBound(100, cycleTime, 1000);
    //timestampDebug(QString("cycleTime %1").arg(cycleTime));
}

bool HantekDsoControl::isRollMode() const {
    return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId] == UINT_MAX;
}

bool HantekDsoControl::isFastRate() const {
    return controlsettings.samplerate.limits == &specification->samplerate.multi;
}

unsigned HantekDsoControl::getRecordLength() const {
    return controlsettings.samplerate.limits->recordLengths[controlsettings.recordLengthId];
}

Dso::ErrorCode HantekDsoControl::retrieveChannelLevelData() {
    // Get channel level data
    // printf( "retrieveChannelLevelData()\n" );
    int errorCode = device->controlRead(&controlsettings.cmdGetLimits);
    if (errorCode < 0) {
        qWarning() << tr("Couldn't get channel level data from oscilloscope");
        emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
        emit communicationError();
        return Dso::ErrorCode::CONNECTION;
    }

    memcpy(controlsettings.offsetLimit, controlsettings.cmdGetLimits.data(),
           sizeof(OffsetsPerGainStep) * specification->channels);
    // printf( "offsetLimit %d %d\n", sizeof(OffsetsPerGainStep), ((unsigned char*)controlsettings.offsetLimit)[0] );

    return Dso::ErrorCode::NONE;
}

unsigned HantekDsoControl::calculateTriggerPoint(unsigned value) {
    unsigned result = value;

    // Each set bit inverts all bits with a lower value
    for (unsigned bitValue = 1; bitValue; bitValue <<= 1)
        if (result & bitValue) result ^= bitValue - 1;

    return result;
}


std::pair<int, unsigned> HantekDsoControl::getCaptureState() const { return std::make_pair(CAPTURE_READY, 0); }


std::vector<unsigned char> HantekDsoControl::getSamples(unsigned &previousSampleCount) const {
    int errorCode;
    errorCode = device->controlWrite(getCommand(ControlCode::CONTROL_ACQUIIRE_HARD_DATA));
    if (errorCode < 0) {
        qWarning() << "Getting sample data failed: " << libUsbErrorString(errorCode);
        emit communicationError();
        return std::vector<unsigned char>();
    }

    unsigned totalSampleCount = this->getSampleCount();
    // qDebug() << "totalSampleCount" << totalSampleCount;
    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if (totalSampleCount < previousSampleCount) {
        std::swap(totalSampleCount, previousSampleCount);
    } else {
        previousSampleCount = totalSampleCount;
    }

    unsigned dataLength = (specification->sampleSize > 8) ? totalSampleCount * 2 : totalSampleCount;

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
    if ( channelUsedChanged ) { // skip the next conversion to avoid artefacts due to channel switch
        channelUsedChanged = false;
        return;
    }
    const size_t totalSampleCount = (specification->sampleSize > 8) ? rawData.size() / 2 : rawData.size();
    QWriteLocker locker(&result.lock);
    result.samplerate = controlsettings.samplerate.current;
    result.append = isRollMode();
    // Prepare result buffers
    result.data.resize(specification->channels);
    for (ChannelID channelCounter = 0; channelCounter < specification->channels; ++channelCounter)
        result.data[channelCounter].clear();

    // Convert channel data
    // ! isFastRate()
    unsigned short activeChannels = specification->channels;
    // Normal mode, channels are using their separate buffers
    for (ChannelID channel = 0; channel < specification->channels; ++channel) {
        result.data[channel].resize(totalSampleCount / specification->channels);

        const unsigned gainID = controlsettings.voltage[channel].gain;
        const unsigned short limit = specification->voltageLimit[channel][gainID];
        const double offset = controlsettings.voltage[channel].offsetReal;
        const double gainStep = specification->gain[gainID].gainSteps;
        const double probeAttn = controlsettings.voltage[channel].probeAttn;
        int shiftDataBuf = 0;
        double gainCalibration = 1.0;

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned bufferPosition = 0;
            // 6022 fast rate
            if ( isFast6022 ) {
                activeChannels = 1;
                if ( channel > 0 ) { // one channel mode only with CH1 (channel == 0)
                    result.data[channel].clear();
                    continue;
                }
            }

            // shift + individual offset for each channel and gain
            shiftDataBuf = specification->voltageOffset[ channel ][ gainID ];
            gainCalibration = 1.0;
            if ( !shiftDataBuf ) { // no config file value
                // get offset value from eeprom[ 8 .. 39 ]
                const unsigned char * pOff = (const unsigned char *) controlsettings.offsetLimit;
                pOff += 2 * gainID + channel; // point to gain/channel offset value in eeprom[ 8 .. 39 ]
                shiftDataBuf = result.samplerate < 30e6 ? *pOff : *(pOff+16); // lowspeed / highspeed
                pOff += 32; // now point to gain/channel gain value in eeprom[ 40 .. 55 ]
                if ( *pOff != 255 && *pOff != 0 ) { // eeprom content valid
                    // byte 128 - 125 ... 128 + 125 -> 1.0 - 0.250 ... 1.0 + 0.250 = 0.75 .. 1.25 
                    gainCalibration = 1.0 + (*pOff - 0x80) / 500.0;
                }
                // printf( "sDB %d, gain_cal %f, ch %d, gIG %d\n", shiftDataBuf, gainCalibration, channel, gainID );
            } 
            // 6022 sample size is (20 + 2) * 1024, set in modelDSO6022.cpp
            // The 1st two or three frames (512 byte) of the raw sample stream are unreliable
            // (Maybe because the common mode input voltage of ADC is handled far out of spec and has to settle)
            // Solution: drop (2048 + 480) heading samples from (22 * 1024) total samples
            // 22 * 1024 - 2048 - 480 = 20000
            const unsigned DROP_DSO6022_HEAD = 2048 + 480;
            if (!isRollMode()) {
                result.data[channel].resize( result.data[channel].size() - DROP_DSO6022_HEAD );
                bufferPosition += DROP_DSO6022_HEAD * activeChannels;
            }
            bufferPosition += channel;
        result.clipped &= ~(0x01 << channel); // clear clipping flag
        for ( unsigned pos = 0; pos < result.data[channel].size();
            ++pos, bufferPosition += activeChannels ) {
            if ( bufferPosition >= totalSampleCount ) 
                bufferPosition %= totalSampleCount; 

            int rawSample = rawData[bufferPosition]; // range 0...255
            if ( rawSample == 0x00 || rawSample == 0xFF ) // min or max -> clipped
                result.clipped |= 0x01 << channel;
            double dataBuf = (double)(rawSample - shiftDataBuf); // int - int
            result.data[channel][pos] = (dataBuf / limit - offset) * gainCalibration * gainStep * probeAttn;
        }
    }
}


unsigned HantekDsoControl::getSampleCount() const {
    if (isRollMode()) {
        // TODO handle libusb error
        return getPacketSize();
    } else {
        if (isFastRate())
            return getRecordLength();
        else
            return getRecordLength() * specification->channels;
    }
}


unsigned HantekDsoControl::updateSamplerate(unsigned downsampler, bool fastRate) {
    qDebug() << "updateSamplerate( " << downsampler << ", " << fastRate << " )";
    // Get samplerate limits
    const ControlSamplerateLimits *limits =
        fastRate ? &specification->samplerate.multi : &specification->samplerate.single;

    // Update settings
    bool fastRateChanged = fastRate != (controlsettings.samplerate.limits == &specification->samplerate.multi);
    if (fastRateChanged) { controlsettings.samplerate.limits = limits; }

    controlsettings.samplerate.downsampler = downsampler;
    if (downsampler)
        controlsettings.samplerate.current = controlsettings.samplerate.limits->base /
                                             specification->bufferDividers[controlsettings.recordLengthId] /
                                             downsampler;
    else
        controlsettings.samplerate.current =
            controlsettings.samplerate.limits->max / specification->bufferDividers[controlsettings.recordLengthId];

    // Update dependencies
    this->setTriggerPosition(controlsettings.trigger.position);

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
    // qDebug() << "restoreTargets()";
    if (controlsettings.samplerate.target.samplerateSet == ControlSettingsSamplerateTarget::Samplerrate)
        this->setSamplerate();
    else
        this->setRecordTime();
}


void HantekDsoControl::updateSamplerateLimits() {
    if (specification->isFixedSamplerateDevice) {
        QList<double> sampleSteps;
        //HORO: limit sample rate if not single channel mode
        double limit = ( isFastRate() || isFast6022 ) ?
            specification->samplerate.single.max : specification->samplerate.multi.max;
        for (auto &v : specification->fixedSampleRates) {
            if ( v.samplerate <= limit ) { sampleSteps << v.samplerate; }
        }
        // qDebug() << sampleSteps;
        emit samplerateSet(1, sampleSteps);
    } else {
        // Works only if the minimum samplerate for normal mode is lower than for fast
        // rate mode, which is the case for all models
        const double min =
            (double)specification->samplerate.single.base / specification->samplerate.single.maxDownsampler;
        const double max = getMaxSamplerate();

        emit samplerateLimitsChanged(min / specification->bufferDividers[controlsettings.recordLengthId],
                                     max / specification->bufferDividers[controlsettings.recordLengthId]);
    }
}


Dso::ErrorCode HantekDsoControl::setSamplerate(double samplerate) {
    // qDebug() << "HDC::setSamplerate(" << samplerate << ")";
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (samplerate == 0.0) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Samplerrate;
    }
    unsigned sampleId;
    for (sampleId = 0; sampleId < specification->fixedSampleRates.size() - 1; ++sampleId)
        if (specification->fixedSampleRates[sampleId].samplerate == samplerate) break;
    modifyCommand<ControlSetTimeDIV>(ControlCode::CONTROL_SETTIMEDIV)
        ->setDiv(specification->fixedSampleRates[sampleId].id);
    controlsettings.samplerate.current = samplerate;
    // Check for Roll mode
    if (!isRollMode())
        emit recordTimeChanged((double)(getRecordLength() - controlsettings.swSampleMargin) /
                               controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setRecordTime(double duration) {
    // printf( "setRecordTime( %g )\n", duration );
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;

    if (duration == 0.0) {
        duration = controlsettings.samplerate.target.duration;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Duration;
    }

    if (!specification->isFixedSamplerateDevice) {
        // Calculate the maximum samplerate that would still provide the requested
        // duration
        double maxSamplerate =
            (double)specification->samplerate.single.recordLengths[controlsettings.recordLengthId] / duration;

        // When possible, enable fast rate if the record time can't be set that low
        // to improve resolution
        bool fastRate = (controlsettings.usedChannels < 1) &&
                        (maxSamplerate >= specification->samplerate.multi.base /
                                              specification->bufferDividers[controlsettings.recordLengthId]);

        // What is the nearest, at most as high samplerate the scope can provide?
        unsigned downsampler = 0;

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return Dso::ErrorCode::PARAMETER;
        else {
            return Dso::ErrorCode::NONE;
        }
    } else { // isFixedSamplerateDevice (e.g. 6022)
        double srLimit;
        if ( isFastRate() || isFast6022 )
            srLimit = (specification->samplerate.single).max;
        else
            srLimit = (specification->samplerate.multi).max;
        // For now - we go for the 20480 size sampling
        // Find highest samplerate using less than 20480 samples to obtain our duration.
        unsigned sampleCount = 20000; // 20480;
        // Ensure that at least 1/2 of remaining samples are available for SW trigger algorithm

        if (specification->isSoftwareTriggerDevice) {
            sampleCount = (sampleCount - controlsettings.swSampleMargin) / 2;
        }
        // qDebug() << "sampleCount" << sampleCount << "limit" << srLimit;
        unsigned sampleId = 0;
        for (unsigned id = 0; id < specification->fixedSampleRates.size(); ++id) {
            double sRate = specification->fixedSampleRates[id].samplerate;
            // qDebug() << "id:" << id << "sRate:" << sRate << "sRate*duration:" << sRate * duration;
            // for stability reason avoid the highest sample rate as default
            if (sRate < srLimit && sRate * duration < sampleCount / 10) {
                sampleId = id;
            }
        }
        // qDebug() << "sampleId:" << sampleId << specification->fixedSampleRates[sampleId].samplerate;
        // Usable sample value
        modifyCommand<ControlSetTimeDIV>(ControlCode::CONTROL_SETTIMEDIV)
            ->setDiv(specification->fixedSampleRates[sampleId].id);
        controlsettings.samplerate.current = specification->fixedSampleRates[sampleId].samplerate;

        emit samplerateChanged(controlsettings.samplerate.current);
        return Dso::ErrorCode::NONE;
    }
}

Dso::ErrorCode HantekDsoControl::setChannelUsed(ChannelID channel, bool used) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    if (channel >= specification->channels)
        return Dso::ErrorCode::PARAMETER;
    // Update settings
    controlsettings.voltage[channel].used = used;
    ChannelID channelCount = 0;
    for (unsigned c = 0; c < specification->channels; ++c) {
        if (controlsettings.voltage[c].used) ++channelCount;
    }
    // Calculate the UsedChannels field for the command
    UsedChannels usedChannels = UsedChannels::USED_CH1;

    if (controlsettings.voltage[1].used) {
        if (controlsettings.voltage[0].used) {
            usedChannels = UsedChannels::USED_CH1CH2;
        } else {
            // DSO-2250 uses a different value for channel 2
            if (specification->cmdSetChannels == BulkCode::BSETCHANNELS)
                usedChannels = UsedChannels::BUSED_CH2;
            else
                usedChannels = UsedChannels::USED_CH2;
        }
    }
    // qDebug() << "usedChannels" << (int)usedChannels;
    if ( usedChannels == UsedChannels::USED_CH1 )
        modifyCommand<ControlSetNumChannels>(ControlCode::CONTROL_SETNUMCHANNELS)->setDiv( 1 );
    else
        modifyCommand<ControlSetNumChannels>(ControlCode::CONTROL_SETNUMCHANNELS)->setDiv( 2 );
    // Check if fast rate mode availability changed
    // bool fastRateChanged = (controlsettings.usedChannels <= 1) != (channelCount <= 1);
    controlsettings.usedChannels = channelCount;
    this->updateSamplerateLimits();
    this->restoreTargets();
    channelUsedChanged = true; // skip next raw samples block to avoid artefacts
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCoupling(ChannelID channel, Dso::Coupling coupling) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification->channels) return Dso::ErrorCode::PARAMETER;

    // SetRelays control command for coupling relays
    if (specification->supportsCouplingRelays) {
        modifyCommand<ControlSetRelays>(ControlCode::CONTROL_SETRELAYS)
            ->setCoupling(channel, coupling != Dso::Coupling::AC);
    }

    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setGain(ChannelID channel, double gain) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    if (channel >= specification->channels) return Dso::ErrorCode::PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainID;
    for (gainID = 0; gainID < specification->gain.size() - 1; ++gainID)
        if (specification->gain[gainID].gainSteps >= gain) break;

    if (channel == 0) {
        modifyCommand<ControlSetVoltDIV_CH1>(ControlCode::CONTROL_SETVOLTDIV_CH1)
            ->setDiv(specification->gain[gainID].gainIndex);
    } else if (channel == 1) {
        modifyCommand<ControlSetVoltDIV_CH2>(ControlCode::CONTROL_SETVOLTDIV_CH2)
            ->setDiv(specification->gain[gainID].gainIndex);
    } else
        qDebug("%s: Unsuported channel: %i\n", __func__, channel);
    controlsettings.voltage[channel].gain = gainID;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setProbe( ChannelID channel, bool probeUsed, double probeAttn ) {
    if (channel >= specification->channels) return Dso::ErrorCode::PARAMETER;
    controlsettings.voltage[channel].probeUsed = probeUsed;
    controlsettings.voltage[channel].probeAttn = probeAttn;
    // printf( "setProbe %g\n", probeAttn );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerMode( %d )\n", (int)mode);
    controlsettings.trigger.mode = mode;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSource(bool special, ChannelID channel) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerSource( %d )\n", channel);
    this->setTriggerLevel(channel, controlsettings.trigger.level[channel]);
    return Dso::ErrorCode::NONE;
}

// trigger level in Volt
Dso::ErrorCode HantekDsoControl::setTriggerLevel(ChannelID channel, double level) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (channel >= specification->channels) return Dso::ErrorCode::PARAMETER;
    //printf("setTriggerLevel( %d, %g )\n", channel, level);
    controlsettings.trigger.level[channel] = level;
    if (!specification->supportsOffset) return Dso::ErrorCode::UNSUPPORTED;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerSlope( %d )\n", (int)slope);
    controlsettings.trigger.slope = slope;
    return Dso::ErrorCode::NONE;
}

// set trigger position (divs from left)
Dso::ErrorCode HantekDsoControl::setTriggerPosition(double position) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerPosition( %g )\n", position);
    controlsettings.trigger.position = position;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCalFreq( double calfreq ) {
    unsigned int cf = (int)calfreq / 1000;
    if ( cf == 0 ) // 50, 100, 200, 500 -> 105, 110, 120, 150
        cf = 100 + calfreq / 10;
    // printf( "HDC::setCalFreq( %g ) -> %d\n", calfreq, cf );
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    // control command for setting
    modifyCommand<ControlSetCalFreq>(ControlCode::CONTROL_SETCALFREQ)
        ->setCalFreq( cf );
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

    if (commandParts[1] == "control") {
        if (!control[codeIndex]) return Dso::ErrorCode::UNSUPPORTED;

        ControlCommand *c = modifyCommand<ControlCommand>((ControlCode)codeIndex);
        hexParse(data, c->data(), c->size());
        return Dso::ErrorCode::NONE;
    } else
        return Dso::ErrorCode::UNSUPPORTED;
}


void HantekDsoControl::addCommand(ControlCommand *newCommand, bool pending) {
    newCommand->pending = pending;
    control[newCommand->code] = newCommand;
    newCommand->next = firstControlCommand;
    firstControlCommand = newCommand;
}


const ControlCommand *HantekDsoControl::getCommand(ControlCode code) const { return control[(uint8_t)code]; }


void HantekDsoControl::run() {
    int errorCode = 0;

    // Send all pending control commands
    ControlCommand *controlCommand = firstControlCommand;
    while (controlCommand) {
        if (controlCommand->pending) {
            timestampDebug(QString("Sending control command %1:%2")
                               .arg(QString::number(controlCommand->code, 16),
                                    hexDump(controlCommand->data(), controlCommand->size())));

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

    {
        std::vector<unsigned char> rawData = this->getSamples(expectedSampleCount);
        if (this->_samplingStarted) { // feed new samples to postprocess and display
            convertRawDataToSamples(rawData);
        } // else don't update, reuse old values
        emit samplesAvailable(&result); // let display run always to allow user interaction
    }

    // Check if we're in single trigger mode
    if (controlsettings.trigger.mode == Dso::TriggerMode::SINGLE && this->_samplingStarted)
        this->enableSampling(false);

    // Sampling completed, restart it when necessary
    this->_samplingStarted = false;

    if (this->sampling) {

        // Sampling hasn't started, update the expected sample count
        expectedSampleCount = this->getSampleCount();

        timestampDebug("Starting to capture");

        this->_samplingStarted = true;
        this->cycleCounter = 0;
        this->startCycle = int(controlsettings.trigger.position * 1000.0 / cycleTime + 1.0);
        this->lastTriggerMode = controlsettings.trigger.mode;
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
