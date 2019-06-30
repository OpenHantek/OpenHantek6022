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


unsigned HantekDsoControl::getChannelCount() const { return specification->channels; }


const ControlSettings *HantekDsoControl::getDeviceSettings() const { return &controlsettings; }


const std::vector<unsigned> &HantekDsoControl::getAvailableRecordLengths() const {
    return controlsettings.samplerate.limits->recordLengths;
}


double HantekDsoControl::getMinSamplerate() const {
    //printf( "getMinSamplerate\n" );
    return (double)specification->samplerate.single.base / specification->samplerate.single.maxDownsampler;
}


double HantekDsoControl::getMaxSamplerate() const {
    //printf( "usedChannels %d\n", controlsettings.usedChannels );
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
    cycleTime = (int)((double)getRecordLength() / controlsettings.samplerate.current * 250);
    // Not more often than every 100 ms though but at least once every second
    cycleTime = qBound(100, cycleTime, 1000);
    //timestampDebug(QString("cycleTime %1").arg(cycleTime));
}


bool HantekDsoControl::isFastRate() const {
    return controlsettings.voltage[0].used && !controlsettings.voltage[1].used;
}


unsigned HantekDsoControl::getRecordLength() const {
    unsigned rawsize = SAMPLESIZE_USED;
    rawsize *= this->downsampling; // take more samples
    rawsize = ( (rawsize + 1024) / 1024 + 2 ) * 1024; // adjust for skipping of minimal 2018 leading samples
    //printf( "getRecordLength: %d\n", rawsize );
    return rawsize;
}


Dso::ErrorCode HantekDsoControl::retrieveChannelLevelData() {
    // Get channel level data
    //printf( "retrieveChannelLevelData()\n" );
    int errorCode = device->controlRead(&controlsettings.cmdGetLimits);
    if (errorCode < 0) {
        qWarning() << tr("Couldn't get channel level data from oscilloscope");
        emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
        emit communicationError();
        return Dso::ErrorCode::CONNECTION;
    }

    memcpy(controlsettings.offsetLimit, controlsettings.cmdGetLimits.data(),
           sizeof(OffsetsPerGainStep) * specification->channels);
    //printf( "offsetLimit %d %d\n", sizeof(OffsetsPerGainStep), ((unsigned char*)controlsettings.offsetLimit)[0] );

    return Dso::ErrorCode::NONE;
}


std::vector<unsigned char> HantekDsoControl::getSamples(unsigned &previousSampleCount) const {
    int errorCode;
    errorCode = device->controlWrite(getCommand(ControlCode::CONTROL_ACQUIIRE_HARD_DATA));
    if (errorCode < 0) {
        qWarning() << "controlWrite: Getting sample data failed: " << libUsbErrorString(errorCode);
        emit communicationError();
        return std::vector<unsigned char>();
    }

    unsigned rawSampleCount = this->getSampleCount();
    //printf( "getSamples, rawSampleCount %d\n", rawSampleCount );
    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if (rawSampleCount < previousSampleCount) {
        std::swap(rawSampleCount, previousSampleCount);
    } else {
        previousSampleCount = rawSampleCount;
    }
    // Save raw data to temporary buffer
    std::vector<unsigned char> data(rawSampleCount);
    int retval = device->bulkReadMulti( data.data(), rawSampleCount );
    if ( retval < 0 ) {
        qWarning() << "bulkReadMulti: Getting sample data failed: " << libUsbErrorString( retval );
        return std::vector<unsigned char>();
    }
    data.resize( (size_t)retval );
    //printf( "bulkReadMulti( %d ) -> %d\n", rawSampleCount, retval );

    static unsigned id = 0;
    ++id;
    timestampDebug(QString("Received packet %1").arg(id));

    return data;
}


void HantekDsoControl::convertRawDataToSamples(const std::vector<unsigned char> &rawData) {
    if ( channelSetupChanged ) { // skip the next conversion to avoid artefacts due to channel switch
        channelSetupChanged = false;
        return;
    }

    const size_t rawSampleCount = isFastRate() ? rawData.size() : (rawData.size() / 2);
    //printf("cRDTS, rawSampleCount %lu\n", rawSampleCount);
    if ( 0 == rawSampleCount ) // nothing to convert
        return;

    QWriteLocker locker(&result.lock);
    result.samplerate = controlsettings.samplerate.current;
    // Prepare result buffers
    result.data.resize(specification->channels);
    for (ChannelID channelCounter = 0; channelCounter < specification->channels; ++channelCounter)
        result.data[channelCounter].clear();

    // The 1st two or three frames (512 byte) of the raw sample stream are unreliable
    // (Maybe because the common mode input voltage of ADC is handled far out of spec and has to settle)
    // Solution: sample at least 2048 more values -> rawSampleSize (must be multiple of 1024)
    //           rawSampleSize = ( ( n*20000 + 1024 ) / 1024 + 2) * 1024;
    // and skip over these samples to get 20000 samples (or n*20000)

    unsigned sampleCount = ((rawSampleCount-1024) / 1000 - 1) * 1000;
    unsigned skipSamples = rawSampleCount - sampleCount;
    unsigned downsampling = sampleCount / SAMPLESIZE_USED;
    //printf("sampleCount %u, downsampling %u\n", sampleCount, downsampling );

    // Convert channel data
    unsigned short activeChannels = specification->channels;
    // Channels are using their separate buffers
    for (ChannelID channel = 0; channel < specification->channels; ++channel) {
        if ( isFastRate() ) { // one channel mode only with CH1 (channel == 0)
            activeChannels = 1;
            if ( channel > 0 ) { // skip unused channel
                result.data[channel].clear();
                continue;
            }
        }
        //result.data[channel].resize(rawSampleCount);
        const unsigned gainID = controlsettings.voltage[channel].gain;
        const unsigned short limit = specification->voltageLimit[channel][gainID];
        const double offset = controlsettings.voltage[channel].offsetReal;
        const double gainStep = specification->gain[gainID].gainSteps;
        const double probeAttn = controlsettings.voltage[channel].probeAttn;
        const double sign = controlsettings.voltage[channel].inverted ? -1.0 : 1.0;
        int shiftDataBuf = 0;
        double gainCalibration = 1.0;

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
            //printf( "sDB %d, gain_cal %f, ch %d, gIG %d\n", shiftDataBuf, gainCalibration, channel, gainID );
        }

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned rawBufferPosition = 0;

        result.data[channel].resize( sampleCount / downsampling );
        rawBufferPosition += skipSamples * activeChannels; // skip first unstable samples
        rawBufferPosition += channel;
        result.clipped &= ~(0x01 << channel); // clear clipping flag
        for ( unsigned index = 0; index < result.data[ channel ].size();
            ++index, rawBufferPosition += activeChannels * downsampling ) { // advance either by one or two blocks
            double sample = 0;
            for ( unsigned iii = 0; iii < downsampling * activeChannels; iii += activeChannels ) {
                int rawSample = rawData[ rawBufferPosition + iii ]; // range 0...255
                if ( rawSample == 0x00 || rawSample == 0xFF ) // min or max -> clipped
                    result.clipped |= 0x01 << channel;
                sample += (double)(rawSample - shiftDataBuf); // int - int
            }
            sample /= downsampling;
            result.data[ channel ][ index ] = sign * (sample / limit - offset) * gainCalibration * gainStep * probeAttn;
        }
    }
}


unsigned HantekDsoControl::getSampleCount() const {
    return isFastRate() ? getRecordLength() : getRecordLength() * specification->channels;
}


unsigned HantekDsoControl::updateSamplerate(unsigned downsampler, bool fastRate) {
    //qDebug() << "updateSamplerate( " << downsampler << ", " << fastRate << " )";
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
    emit recordTimeChanged((double)getRecordLength() / controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    return downsampler;
}


void HantekDsoControl::restoreTargets() {
    //qDebug() << "restoreTargets()";
    if (controlsettings.samplerate.target.samplerateSet == ControlSettingsSamplerateTarget::Samplerrate)
        this->setSamplerate();
    else
        this->setRecordTime();
}


void HantekDsoControl::updateSamplerateLimits() {
    QList<double> sampleSteps;
    double limit = ( isFastRate() ) ?
        specification->samplerate.single.max : specification->samplerate.multi.max;

    if ( controlsettings.samplerate.current > limit ) {
        setSamplerate( limit );
    }
    for (auto &v : specification->fixedSampleRates) {
        if ( v.samplerate <= limit ) { sampleSteps << v.samplerate; }
    }
    //qDebug() << "HDC::updateSamplerateLimits " << sampleSteps;
    //restoreTargets();
    emit samplerateSet(1, sampleSteps);
}


Dso::ErrorCode HantekDsoControl::setSamplerate(double samplerate) {
    //printf( "HDC::setSamplerate( %g )\n", samplerate );
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
    setDownsampling( specification->fixedSampleRates[sampleId].downsampling );
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    // Check for Roll mode
    emit recordTimeChanged((double)(getRecordLength() - controlsettings.swSampleMargin) /
                           controlsettings.samplerate.current);
    emit samplerateChanged(controlsettings.samplerate.current);

    return Dso::ErrorCode::NONE;
}


double HantekDsoControl::getSamplerate() const {
    return controlsettings.samplerate.current;
}


unsigned HantekDsoControl::getSamplesize() const {
    return SAMPLESIZE_USED;
}


Dso::ErrorCode HantekDsoControl::setRecordTime(double duration) {
    //printf( "setRecordTime( %g )\n", duration );
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;

    if (duration == 0.0) {
        duration = controlsettings.samplerate.target.duration;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Duration;
    }
    //printf( "duration = %g\n", duration );

    double srLimit;
    if ( isFastRate() )
        srLimit = (specification->samplerate.single).max;
    else
        srLimit = (specification->samplerate.multi).max;
    // For now - we go for the SAMPLESIZE_USED (= 20000) size sampling, defined in model6022.h
    // Find highest samplerate using less equal half of these samples to obtain our duration.
    unsigned sampleId = 0;
    for (unsigned id = 0; id < specification->fixedSampleRates.size(); ++id) {
        double sRate = specification->fixedSampleRates[id].samplerate;
        //qDebug() << "id:" << id << "sRate:" << sRate << "sRate*duration:" << sRate * duration;
        // Ensure that at least 1/2 of remaining samples are available for SW trigger algorithm
        // for stability reason avoid the highest sample rate as default
        if ( sRate < srLimit && sRate * duration <= SAMPLESIZE_USED / 2 ) {
            sampleId = id;
        }
    }
    double samplerate = specification->fixedSampleRates[sampleId].samplerate;
    //qDebug() << "sampleId:" << sampleId << srLimit << samplerate;
    // Usable sample value
    modifyCommand<ControlSetTimeDIV>(ControlCode::CONTROL_SETTIMEDIV)
        ->setDiv(specification->fixedSampleRates[sampleId].id);
    controlsettings.samplerate.current = samplerate;
    setDownsampling( specification->fixedSampleRates[sampleId].downsampling );
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    emit samplerateChanged( samplerate );
    return Dso::ErrorCode::NONE;
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
            usedChannels = UsedChannels::USED_CH2;
        }
    }
    //qDebug() << "usedChannels" << (int)usedChannels;
    if ( usedChannels == UsedChannels::USED_CH1 )
        modifyCommand<ControlSetNumChannels>(ControlCode::CONTROL_SETNUMCHANNELS)->setDiv( 1 );
    else
        modifyCommand<ControlSetNumChannels>(ControlCode::CONTROL_SETNUMCHANNELS)->setDiv( 2 );
    // Check if fast rate mode availability changed
    controlsettings.usedChannels = channelCount;
    this->updateSamplerateLimits();
    this->restoreTargets();
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setChannelInverted(ChannelID channel, bool inverted) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    if (channel >= specification->channels)
        return Dso::ErrorCode::PARAMETER;
    // Update settings
    //printf("setChannelInverted %s\n", inverted?"true":"false");
    controlsettings.voltage[channel].inverted = inverted;
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setGain(ChannelID channel, double gain) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;

    if (channel >= specification->channels)
        return Dso::ErrorCode::PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainID;
    for (gainID = 0; gainID < specification->gain.size() - 1; ++gainID)
        if (specification->gain[gainID].gainSteps >= gain) 
            break;

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
    if (channel >= specification->channels)
        return Dso::ErrorCode::PARAMETER;
    controlsettings.voltage[channel].probeUsed = probeUsed;
    controlsettings.voltage[channel].probeAttn = probeAttn;
    //printf( "setProbe %g\n", probeAttn );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    controlsettings.trigger.mode = mode;
    if ( Dso::TriggerMode::SINGLE != mode )
        enableSampling( true );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSource(ChannelID channel) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerSource( %d )\n", channel);
    controlsettings.trigger.source = channel;
    return Dso::ErrorCode::NONE;
}

// trigger level in Volt
Dso::ErrorCode HantekDsoControl::setTriggerLevel(ChannelID channel, double level) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    if (channel >= specification->channels)
        return Dso::ErrorCode::PARAMETER;
    //printf("setTriggerLevel( %d, %g )\n", channel, level);
    controlsettings.trigger.level[channel] = level;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerSlope( %d )\n", (int)slope);
    controlsettings.trigger.slope = slope;
    return Dso::ErrorCode::NONE;
}


// set trigger position (0.0 - 1.0)
Dso::ErrorCode HantekDsoControl::setTriggerPosition(double position) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    //printf("setTriggerPosition( %g )\n", position);
    controlsettings.trigger.position = position;
    return Dso::ErrorCode::NONE;
}


int HantekDsoControl::softwareTrigger() {
    ChannelID channel = controlsettings.trigger.source;
    // Trigger channel not in use
    if (!controlsettings.voltage[channel].used || result.data.empty()) {
        return result.triggerPosition = -1;
    }
    //printf( "softwareTrigger()\n" );
    const unsigned swTriggerThreshold = 5;                ///< Software trigger, threshold
    const unsigned swTriggerSampleSet = 11;               ///< Software trigger, sample set
    unsigned int preTrigSamples = 0;
    unsigned int postTrigSamples = 0;
    unsigned int swTriggerStart = 0;
    int triggerPosition = -1;
    result.triggerPosition = -1;
    const std::vector<double> &samples = result.data[channel];
    double level = controlsettings.trigger.level[channel];
    size_t sampleCount = samples.size(); // number of available samples
    double timeDisplay = controlsettings.samplerate.target.duration; // time for full screen width
    double sampleRate = controlsettings.samplerate.current;
    double samplesDisplay = timeDisplay * sampleRate;
    // samples for full screen width
    //printf( "sC %lu, tD %g, sR %g, sD %g\n", sampleCount, timeDisplay, sampleRate, samplesDisplay );
    if (samplesDisplay >= sampleCount) {
        // For sure not enough samples to adjust for jitter.
        // Following options exist:
        //    1: Decrease sample rate
        //    2: Change trigger mode to auto
        //    3: Ignore samples
        // For now #3 is chosen
        timestampDebug(QString("Too few samples to make a steady "
                               "picture. Decrease sample rate"));
        return result.triggerPosition = -1;
    }

    preTrigSamples = (unsigned)(controlsettings.trigger.position * samplesDisplay); // samples left of trigger
    postTrigSamples = (unsigned)sampleCount - ((unsigned)samplesDisplay - preTrigSamples); // samples right of trigger
    // |-----------samples-----------| // available sample
    // |--disp--|                      // display size
    // |<<<<<T>>|--------------------| // >> = right = (disp-pre) i.e. right of trigger on screen
    // |<pre<|                         // << = left = pre
    // |--(samp-(disp-pre))-------|>>|
    // |<<<<<|????????????????????|>>| // ?? = search for trigger in this range [left,right]
    double prev;
    bool (*opcmp)(double,double,double);
    bool (*smplcmpBefore)(double,double);
    bool (*smplcmpAfter)(double,double);
    // define trigger condition
    if (controlsettings.trigger.slope == Dso::Slope::Positive) {
        prev = INT_MAX;
        opcmp = [](double value, double level, double prev) { return value > level && prev <= level;};
        smplcmpBefore = [](double sampleK, double value) { return sampleK < value;};
        smplcmpAfter = [](double sampleK, double value) { return sampleK >= value;};
    } else {
        prev = INT_MIN;
        opcmp = [](double value, double level, double prev) { return value < level && prev >= level;};
        smplcmpBefore = [](double sampleK, double value) { return sampleK >= value;};
        smplcmpAfter = [](double sampleK, double value) { return sampleK < value;};
    }
    // search for trigger point in a range that leaves enough samples left and right of trigger for display
    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
        double value = samples[i];
        if (opcmp(value, level, prev)) { // trigger condition met
            // check for the next few SampleSet samples, if they are also above/below the trigger value
            unsigned int risingBefore = 0;
            for (unsigned int k = i - 1; k > i - swTriggerSampleSet && k > 0; k--) {
                if (smplcmpBefore(samples[k], level))
                    risingBefore++;
            }
            unsigned int risingAfter = 0;
            for (unsigned int k = i + 1; k < i + swTriggerSampleSet && k < sampleCount; k++) {
                if (smplcmpAfter(samples[k], level))
                    risingAfter++;
            }

            // if at least >Threshold (=5) samples before and after trig meet the condition, set trigger
            if (risingBefore > swTriggerThreshold && risingAfter > swTriggerThreshold) {
                swTriggerStart = i-1;
                break;
            }
        }
        prev = value;
    }
    if (swTriggerStart == 0) {
        // timestampDebug(QString("Trigger not asserted. Data ignored"));
        preTrigSamples = 0; // preTrigSamples may never be greater than swTriggerStart
        postTrigSamples = 0;
    }
    //printf("PPS(%d %d %d)\n", preTrigSamples, postTrigSamples, swTriggerStart);

    if (postTrigSamples > preTrigSamples)
        triggerPosition = ( swTriggerStart - preTrigSamples );
    else
        triggerPosition = -1;

    return result.triggerPosition = triggerPosition;
}


void HantekDsoControl::triggering() {
    //printf( "HDC::triggering()\n" );
    static DSOsamples triggeredResult; // storage for last triggered trace samples
    if ( result.triggerPosition >= 0 ) { // live trace has triggered
        // Use this trace and save it also
        triggeredResult.data = result.data;
        triggeredResult.samplerate = result.samplerate;
        triggeredResult.clipped = result.clipped;
        triggeredResult.triggerPosition = result.triggerPosition;
        result.liveTrigger = true; // show green "TR" top left 
    } else if ( controlsettings.trigger.mode == Dso::TriggerMode::NORMAL ) { // Not triggered in NORMAL mode
        // Use saved trace (even if it is empty)
        result.data = triggeredResult.data; 
        result.samplerate = triggeredResult.samplerate;
        result.clipped = triggeredResult.clipped;
        result.triggerPosition = triggeredResult.triggerPosition;
        result.liveTrigger = false; // show red "TR" top left
    } else { // Not triggered and not NORMAL mode
        // Use the free running trace, discard history
        triggeredResult.data.clear(); // discard trace
        triggeredResult.triggerPosition = -1; // not triggered
        result.liveTrigger = false; // show red "TR" top left
    }
}


Dso::ErrorCode HantekDsoControl::setCalFreq( double calfreq ) {
    unsigned int cf = (int)calfreq / 1000; // 1000, ..., 100000 -> 1, ..., 100
    if ( cf == 0 ) // 50, 100, 200, 500 -> 105, 110, 120, 150
        cf = 100 + calfreq / 10;
    //printf( "HDC::setCalFreq( %g ) -> %d\n", calfreq, cf );
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;
    // control command for setting
    modifyCommand<ControlSetCalFreq>(ControlCode::CONTROL_SETCALFREQ)
        ->setCalFreq( cf );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::stringCommand(const QString &commandString) {
    if (!device->isConnected())
        return Dso::ErrorCode::CONNECTION;

    QStringList commandParts = commandString.split(' ', QString::SkipEmptyParts);

    if (commandParts.count() < 1)
        return Dso::ErrorCode::PARAMETER;
    if (commandParts[0] != "send")
        return Dso::ErrorCode::UNSUPPORTED;
    if (commandParts.count() < 2)
        return Dso::ErrorCode::PARAMETER;

    uint8_t codeIndex = 0;
    hexParse(commandParts[2], &codeIndex, 1);
    QString data = commandString.section(' ', 2, -1, QString::SectionSkipEmpty);

    if (commandParts[1] == "control") {
        if (!control[codeIndex])
            return Dso::ErrorCode::UNSUPPORTED;

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
    {
        std::vector<unsigned char> rawData = this->getSamples(expectedSampleCount);
        if (this->_samplingStarted) { // feed new samples to postprocess and display
            convertRawDataToSamples(rawData);
            softwareTrigger(); // detect trigger point of latest samples
            triggering();      // present either free running or last triggered trace
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
