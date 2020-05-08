// SPDX-License-Identifier: GPL-2.0+

// #define TIMESTAMPDEBUG

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

#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "models/modelDSO6022.h"
#include "scopesettings.h"
#include "usb/usbdevice.h"

using namespace Hantek;
using namespace Dso;


HantekDsoControl::HantekDsoControl( USBDevice *device, const DSOModel *model )
    : device( device ), model( model ), specification( model->spec() ),
      controlsettings( &( specification->samplerate.single ), specification->channels ) {
    qRegisterMetaType< DSOsamples * >();

    if ( device && specification->fixedUSBinLength )
        device->overwriteInPacketLength( unsigned( specification->fixedUSBinLength ) );
    // Apply special requirements by the devices model
    model->applyRequirements( this );
    retrieveChannelLevelData();
}


HantekDsoControl::~HantekDsoControl() {
    while ( firstControlCommand ) {
        ControlCommand *t = firstControlCommand->next;
        delete firstControlCommand;
        firstControlCommand = t;
    }
}


bool HantekDsoControl::deviceIsConnected() { return device ? device->isConnected() : true; }


void HantekDsoControl::restoreTargets() {
    // qDebug() << "restoreTargets()";
    if ( controlsettings.samplerate.target.samplerateSet == ControlSettingsSamplerateTarget::Samplerrate )
        this->setSamplerate();
    else
        this->setRecordTime();
}


void HantekDsoControl::updateSamplerateLimits() {
    QList< double > sampleSteps;
    double limit = isFastRate() ? specification->samplerate.single.max : specification->samplerate.multi.max;

    if ( controlsettings.samplerate.current > limit ) {
        setSamplerate( limit );
    }
    for ( auto &v : specification->fixedSampleRates ) {
        if ( v.samplerate <= limit ) {
            sampleSteps << v.samplerate;
        }
    }
    // qDebug() << "HDC::updateSamplerateLimits " << sampleSteps;
    emit samplerateSet( 1, sampleSteps );
}


Dso::ErrorCode HantekDsoControl::setSamplerate( double samplerate ) {
    // printf( "HDC::setSamplerate( %g )\n", samplerate );
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( samplerate == 0.0 ) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Samplerrate;
    }
    unsigned sampleId;
    for ( sampleId = 0; sampleId < specification->fixedSampleRates.size() - 1; ++sampleId ) {
        if ( long( round( specification->fixedSampleRates[ sampleId ].samplerate ) ) ==
             long( round( samplerate ) ) ) // dont compare double == double
            break;
    }
    modifyCommand< ControlSetTimeDIV >( ControlCode::CONTROL_SETTIMEDIV )->setDiv( specification->fixedSampleRates[ sampleId ].id );
    controlsettings.samplerate.current = samplerate;
    setDownsampling( specification->fixedSampleRates[ sampleId ].downsampling );
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    // Check for Roll mode
    emit recordTimeChanged( double( getRecordLength() - controlsettings.swSampleMargin ) / controlsettings.samplerate.current );
    emit samplerateChanged( controlsettings.samplerate.current );

    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setRecordTime( double duration ) {
    // printf( "setRecordTime( %g )\n", duration );
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( duration == 0.0 ) {
        duration = controlsettings.samplerate.target.duration;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Duration;
    }
    // printf( "duration = %g\n", duration );

    double srLimit;
    if ( isFastRate() )
        srLimit = ( specification->samplerate.single ).max;
    else
        srLimit = ( specification->samplerate.multi ).max;
    // For now - we go for the SAMPLESIZE_USED (= 20000) size sampling, defined in model6022.h
    // Find highest samplerate using less equal half of these samples to obtain our duration.
    unsigned sampleId = 0;
    for ( unsigned id = 0; id < specification->fixedSampleRates.size(); ++id ) {
        double sRate = specification->fixedSampleRates[ id ].samplerate;
        // qDebug() << "id:" << id << "sRate:" << sRate << "sRate*duration:" << sRate * duration;
        // Ensure that at least 1/2 of remaining samples are available for SW trigger algorithm
        // for stability reason avoid the highest sample rate as default
        if ( sRate < srLimit && sRate * duration <= SAMPLESIZE_USED / 2 ) {
            sampleId = id;
        }
    }
    double samplerate = specification->fixedSampleRates[ sampleId ].samplerate;
    // qDebug() << "HDC::sRT: sampleId:" << sampleId << srLimit << samplerate;
    // Usable sample value
    modifyCommand< ControlSetTimeDIV >( ControlCode::CONTROL_SETTIMEDIV )->setDiv( specification->fixedSampleRates[ sampleId ].id );
    controlsettings.samplerate.current = samplerate;
    setDownsampling( specification->fixedSampleRates[ sampleId ].downsampling );
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    emit samplerateChanged( samplerate );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCalFreq( double calfreq ) {
    unsigned int cf = unsigned( calfreq ) / 1000; // 1000, ..., 100000 -> 1, ..., 100
    if ( cf == 0 ) {                              // 50, 60, 100, 200, 500 -> 105, 106, 110, 120, 150
        cf = 100 + unsigned( calfreq ) / 10;
        if ( 110 == cf ) // special case for sigrok FW (e.g. DDS120) 100Hz -> 0
            cf = 0;
    }
    // printf( "HDC::setCalFreq( %g ) -> %d\n", calfreq, cf );
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    // control command for setting
    modifyCommand< ControlSetCalFreq >( ControlCode::CONTROL_SETCALFREQ )->setCalFreq( uint8_t( cf ) );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setChannelUsed( ChannelID channel, bool used ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    // Update settings
    controlsettings.voltage[ channel ].used = used;
    // Calculate the UsedChannels field for the command
    UsedChannels usedChannels = UsedChannels::USED_CH1;
    controlsettings.channelCount = 1;

    if ( controlsettings.voltage[ 1 ].used ) {
        controlsettings.channelCount = 2;
        if ( controlsettings.voltage[ 0 ].used ) {
            usedChannels = UsedChannels::USED_CH1CH2;
        } else {
            usedChannels = UsedChannels::USED_CH2;
        }
    }
    setFastRate( usedChannels == UsedChannels::USED_CH1 );
    // qDebug() << "usedChannels" << int( usedChannels );
    modifyCommand< ControlSetNumChannels >( ControlCode::CONTROL_SETNUMCHANNELS )->setDiv( isFastRate() ? 1 : 2 );
    // Check if fast rate mode availability changed
    this->updateSamplerateLimits();
    this->restoreTargets();
    channelSetupChanged = true; // skip next raw samples block to avoid artefacts
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setChannelInverted( ChannelID channel, bool inverted ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    // Update settings
    // printf("setChannelInverted %s\n", inverted?"true":"false");
    controlsettings.voltage[ channel ].inverted = inverted;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setGain( ChannelID channel, double gain ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;

    gain /= controlsettings.voltage[ channel ].probeAttn; // gain needs to be scaled by probe attenuation
    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainID;
    for ( gainID = 0; gainID < specification->gain.size() - 1; ++gainID )
        if ( specification->gain[ gainID ].gainSteps >= gain )
            break;

    if ( channel == 0 ) {
        modifyCommand< ControlSetVoltDIV_CH1 >( ControlCode::CONTROL_SETVOLTDIV_CH1 )
            ->setDiv( specification->gain[ gainID ].gainIndex );
    } else if ( channel == 1 ) {
        modifyCommand< ControlSetVoltDIV_CH2 >( ControlCode::CONTROL_SETVOLTDIV_CH2 )
            ->setDiv( specification->gain[ gainID ].gainIndex );
    } else
        qDebug( "%s: Unsupported channel: %i\n", __func__, channel );
    controlsettings.voltage[ channel ].gain = gainID;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setProbe( ChannelID channel, double probeAttn ) {
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    controlsettings.voltage[ channel ].probeAttn = probeAttn;
    // printf( "setProbe %g\n", probeAttn );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCoupling( ChannelID channel, Dso::Coupling coupling ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;

    if ( hasCommand( ControlCode::CONTROL_SETCOUPLING ) ) // don't send command if it is not implemented (like on the 6022)
        modifyCommand< ControlSetCoupling >( ControlCode::CONTROL_SETCOUPLING )
            ->setCoupling( channel, coupling == Dso::Coupling::DC );
    controlsettings.voltage[ channel ].coupling = coupling;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerMode( Dso::TriggerMode mode ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    controlsettings.trigger.mode = mode;
    if ( Dso::TriggerMode::SINGLE != mode )
        enableSampling( true );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSource( ChannelID channel, bool smooth ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    // printf("setTriggerSource( %d, %d )\n", channel, smooth);
    controlsettings.trigger.source = channel;
    controlsettings.trigger.smooth = smooth;
    return Dso::ErrorCode::NONE;
}

// trigger level in Volt
Dso::ErrorCode HantekDsoControl::setTriggerLevel( ChannelID channel, double level ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    // printf("setTriggerLevel( %d, %g )\n", channel, level);
    controlsettings.trigger.level[ channel ] = level;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSlope( Dso::Slope slope ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    // printf("setTriggerSlope( %d )\n", (int)slope);
    controlsettings.trigger.slope = slope;
    return Dso::ErrorCode::NONE;
}


// set trigger position (0.0 - 1.0)
Dso::ErrorCode HantekDsoControl::setTriggerOffset( double position ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;
    // printf("setTriggerPosition( %g )\n", position);
    controlsettings.trigger.position = position;
    return Dso::ErrorCode::NONE;
}


// Initialize the device with the current settings.
void HantekDsoControl::applySettings( DsoSettingsScope *scope ) {
    bool mathUsed = scope->anyUsed( specification->channels );
    for ( ChannelID channel = 0; channel < specification->channels; ++channel ) {
        setProbe( channel, scope->voltage[ channel ].probeAttn );
        setGain( channel, scope->gain( channel ) * DIVS_VOLTAGE );
        setTriggerLevel( channel, scope->voltage[ channel ].trigger );
        setChannelUsed( channel, mathUsed | scope->anyUsed( channel ) );
        setChannelInverted( channel, scope->voltage[ channel ].inverted );
        setCoupling( channel, Dso::Coupling( scope->voltage[ channel ].couplingOrMathIndex ) );
    }

    setRecordTime( scope->horizontal.timebase * DIVS_TIME );
    setCalFreq( scope->horizontal.calfreq );
    setTriggerMode( scope->trigger.mode );
    setTriggerOffset( scope->trigger.offset );
    setTriggerSlope( scope->trigger.slope );
    setTriggerSource( scope->trigger.source, scope->trigger.smooth );
}


/// \brief Start sampling process.
void HantekDsoControl::enableSampling( bool enabled ) {
    sampling = enabled;
    updateSamplerateLimits();
    emit samplingStatusChanged( enabled );
}


unsigned HantekDsoControl::getRecordLength() const {
    unsigned rawsize = SAMPLESIZE_USED;
    rawsize *= this->downsamplingNumber;                // take more samples
    rawsize = ( ( rawsize + 1024 ) / 1024 + 2 ) * 1024; // adjust for skipping of minimal 2048 leading samples
    // printf( "getRecordLength: %d\n", rawsize );
    return rawsize;
}


Dso::ErrorCode HantekDsoControl::retrieveChannelLevelData() {
    // Get calibration data from EEPROM
    // printf( "retrieveChannelLevelData()\n" );
    int errorCode = -1;
    if ( device && specification->hasCalibrationEEPROM )
        errorCode = device->controlRead( &controlsettings.cmdGetLimits );
    if ( errorCode < 0 ) {
        // invalidate the calibration values.
        memset( controlsettings.calibrationValues, 0xFF, sizeof( CalibrationValues ) );
        if ( device ) {
            QString message = tr( "Couldn't get calibration data from oscilloscope's EEPROM. Use a config file for calibration!" );
            qWarning() << message;
            emit statusMessage( message, 0 );
            emit communicationError();
            return Dso::ErrorCode::CONNECTION;
        } else {
            return Dso::ErrorCode::NONE;
        }
    }
    memcpy( controlsettings.calibrationValues, controlsettings.cmdGetLimits.data(), sizeof( CalibrationValues ) );
    //     printf("HDC::cV: %lu, %d, %g\n", sizeof( CalibrationValues ),
    //         controlsettings.calibrationValues->off.ls.step[ 7 ][ 0 ] - 0x80,
    //         (controlsettings.calibrationValues->fine.ls.step[ 7 ][ 0 ] - 0x80) / 250.0
    //     );
    return Dso::ErrorCode::NONE;
}


void HantekDsoControl::stopSampling() {
    if ( !device )
        return;
    auto controlCommand = ControlStopSampling();
    timestampDebug( QString( "Sending control command %1:%2" )
                        .arg( QString::number( controlCommand.code, 16 ),
                              hexDump( controlCommand.data(), unsigned( controlCommand.size() ) ) ) );
    int errorCode = device->controlWrite( &controlCommand );
    if ( errorCode < 0 ) {
        qWarning() << "controlWrite: stop sampling failed: " << libUsbErrorString( errorCode );
        emit communicationError();
    }
}


std::vector< unsigned char > HantekDsoControl::getSamples( unsigned &previousSampleCount ) const {
    int errorCode;
    errorCode = device->controlWrite( getCommand( ControlCode::CONTROL_STARTSAMPLING ) );
    if ( errorCode < 0 ) {
        qWarning() << "controlWrite: Getting sample data failed: " << libUsbErrorString( errorCode );
        emit communicationError();
        return std::vector< unsigned char >();
    }

    unsigned rawSampleCount = this->getSampleCount();
    // printf( "getSamples, rawSampleCount %d\n", rawSampleCount );
    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if ( rawSampleCount < previousSampleCount ) {
        std::swap( rawSampleCount, previousSampleCount );
    } else {
        previousSampleCount = rawSampleCount;
    }
    // Save raw data to temporary buffer
    std::vector< unsigned char > data( rawSampleCount );
    int retval = device->bulkReadMulti( data.data(), rawSampleCount );
    if ( retval < 0 ) {
        qWarning() << "bulkReadMulti: Getting sample data failed: " << libUsbErrorString( retval );
        return std::vector< unsigned char >();
    }
    data.resize( size_t( retval ) );
    // printf( "bulkReadMulti( %d ) -> %d\n", rawSampleCount, retval );

    static unsigned id = 0;
    ++id;
    timestampDebug( QString( "Received packet %1" ).arg( id ) );

    return data;
}


std::vector< unsigned char > HantekDsoControl::getDummySamples( unsigned &previousSampleCount ) const {
    const uint8_t V_zero = 128;   // ADC = 0V
    const uint8_t V_plus_1 = 153; // ADC = 1V
    const uint8_t V_plus_2 = 178; // ADC = 2V
    const uint8_t V_minus_2 = 78; // ADC = -2V
    static uint8_t ch1 = V_zero;
    static uint8_t ch2 = V_zero;
    static int counter = 0;
    unsigned rawSampleCount = this->getSampleCount();
    if ( rawSampleCount < previousSampleCount ) {
        std::swap( rawSampleCount, previousSampleCount );
    } else {
        previousSampleCount = rawSampleCount;
    }
    std::vector< unsigned char > data( rawSampleCount );
    auto end = data.end();
    const int deltaT = 99;
    for ( auto it = data.begin(); it != end; ++it ) {
        // deltaT (=99) defines the frequency of the dummy signals:
        // ch1 = 1 kHz and ch2 = 500 Hz for samplerate = 100 kS/s .. 10 MS/s
        // for samplerates below this range the frequency decreases proportional:
        // - samplerate = 50 kS/s -> 500 Hz / 250 Hz
        // - samplerate = 20 kS/s -> 200 Hz / 100 Hz
        // - samplerate = 10 kS/s -> 100 Hz / 50 Hz
        // similar if you're above:
        // - samplerate = 30 MS/s -> 3000 Hz (only ch1)
        // - samplerate = 24 MS/s -> 2400 Hz (only ch1)
        // - samplerate = 15 MS/s -> 1500 Hz / 750 Hz
        // - samplerate = 12 MS/s -> 1200 Hz / 600 Hz
        if ( ++counter >= deltaT ) {
            counter = 0;
            if ( --ch1 < V_minus_2 ) {
                ch1 = V_plus_2;
                ch2 = ch2 <= V_plus_1 ? V_plus_2 : V_zero;
            }
        }
        *it = ch1;
        if ( !isFastRate() ) {
            *++it = ch2;
        }
    }

    static unsigned id = 0;
    ++id;
    timestampDebug( QString( "Received dummy packet %1" ).arg( id ) );

    return data;
}


void HantekDsoControl::convertRawDataToSamples( const std::vector< unsigned char > &rawData, unsigned activeChannels ) {
    if ( channelSetupChanged ) { // skip the next conversion to avoid artefacts due to channel switch
        channelSetupChanged = false;
        return;
    }
    if ( isFastRate() != ( 1 == activeChannels ) ) // avoid possible race condition
        return;

    // The 1st two or three frames (512 byte) of the raw sample stream are unreliable
    // (Maybe because the common mode input voltage of ADC is handled far out of spec and has to settle)
    // Solution: sample at least 2048 more values -> rawSampleSize (must be multiple of 1024)
    //           rawSampleSize = ( ( n*20000 + 1024 ) / 1024 + 2) * 1024;
    // and skip over these additional samples to get 20000 samples (or n*20000 if oversampled)

    const unsigned rawSampleCount = unsigned( rawData.size() ) / activeChannels;
    // TODO: is this needed? rawSampleCount should always be > SAMPLESIZE_USED (=20000)
    const unsigned sampleCount = ( rawSampleCount > 1024 ) ? ( ( rawSampleCount - 1024 ) / 1000 - 1 ) * 1000 : rawSampleCount;
    const unsigned rawDownsampling = sampleCount / SAMPLESIZE_USED;
    // qDebug() << "HDC::cRDTS rawSampleCount sampleCount:" << rawSampleCount << sampleCount;
    const unsigned skipSamples = rawSampleCount - sampleCount;

    QWriteLocker locker( &result.lock );
    result.samplerate = controlsettings.samplerate.current;
    // Prepare result buffers
    result.data.resize( specification->channels );
    for ( ChannelID channelCounter = 0; channelCounter < specification->channels; ++channelCounter )
        result.data[ channelCounter ].clear();

    // The 1st two or three frames (512 byte) of the raw sample stream are unreliable
    // (Maybe because the common mode input voltage of ADC is handled far out of spec and has to settle)
    // Solution: sample at least 2048 more values -> rawSampleSize (must be multiple of 1024)
    //           rawSampleSize = ( ( n*20000 + 1024 ) / 1024 + 2) * 1024;
    // and skip over these samples to get 20000 samples (or n*20000)

    // printf("sampleCount %u, downsampling %u\n", sampleCount, downsampling );

    // Convert channel data
    // Channels are using their separate buffers
    for ( ChannelID channel = 0; channel < activeChannels; ++channel ) {
        const unsigned gainID = controlsettings.voltage[ channel ].gain;
        const int voltageScale = specification->voltageScale[ channel ][ gainID ];
        const double gainStep = specification->gain[ gainID ].gainSteps;
        const double probeAttn = controlsettings.voltage[ channel ].probeAttn;
        const double sign = controlsettings.voltage[ channel ].inverted ? -1.0 : 1.0;

        // shift + individual offset for each channel and gain
        double gainCalibration = 1.0;
        double voltageOffset = specification->voltageOffset[ channel ][ gainID ];
        if ( !bool( voltageOffset ) ) { // no config file value
            // get offset value from eeprom[ 8 .. 39 and (if available) 56 .. 87]
            int offsetRaw;
            int offsetFine;
            if ( result.samplerate < 30e6 ) {
                offsetRaw = controlsettings.calibrationValues->off.ls.step[ gainID ][ channel ];
                offsetFine = controlsettings.calibrationValues->fine.ls.step[ gainID ][ channel ];
            } else {
                offsetRaw = controlsettings.calibrationValues->off.hs.step[ gainID ][ channel ];
                offsetFine = controlsettings.calibrationValues->fine.hs.step[ gainID ][ channel ];
            }
            if ( offsetRaw && offsetRaw != 255 && offsetFine && offsetFine != 255 ) { // data valid
                voltageOffset = offsetRaw + ( offsetFine - 0x80 ) / 250.0;
            } else {                  // no offset correction
                voltageOffset = 0x80; // ADC has "binary offset" format (0x80 = 0V)
            }
            int gain = controlsettings.calibrationValues->gain.step[ gainID ][ channel ];
            if ( gain && gain != 255 ) { // data valid
                gainCalibration = 1.0 + ( gain - 0x80 ) / 500.0;
            }
            // printf( "sDB %d, gC %f, ch %d, gID %d\n", shiftDataBuf, gainCalibration, channel, gainID );
        }

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned rawBufferPosition = 0;

        result.data[ channel ].resize( sampleCount / rawDownsampling );
        rawBufferPosition += skipSamples * activeChannels; // skip first unstable samples
        rawBufferPosition += channel;
        result.clipped &= ~( 0x01 << channel ); // clear clipping flag
        for ( unsigned index = 0; index < result.data[ channel ].size();
              ++index, rawBufferPosition += activeChannels * rawDownsampling ) { // advance either by one or two blocks
            double sample = 0.0;
            for ( unsigned iii = 0; iii < rawDownsampling * activeChannels; iii += activeChannels ) {
                int rawSample = rawData[ rawBufferPosition + iii ]; // range 0...255
                if ( rawSample == 0x00 || rawSample == 0xFF )       // min or max -> clipped
                    result.clipped |= 0x01 << channel;
                sample += double( rawSample ) - voltageOffset;
            }
            sample /= rawDownsampling;
            result.data[ channel ][ index ] = sign * sample / voltageScale * gainCalibration * gainStep * probeAttn;
        }
    }
}


// search for trigger point from defined point, default startPos = 0;
unsigned HantekDsoControl::searchTriggerPoint( Dso::Slope dsoSlope, unsigned int startPos ) {
    int slope;
    if ( dsoSlope == Dso::Slope::Positive )
        slope = 1;
    else if ( dsoSlope == Dso::Slope::Negative )
        slope = -1;
    else
        return 0;

    ChannelID channel = controlsettings.trigger.source;
    const std::vector< double > &samples = result.data[ channel ];
    unsigned sampleCount = unsigned( samples.size() ); ///< number of available samples
    // printf("searchTriggerPoint( %d, %d )\n", (int)dsoSlope, startPos );
    if ( startPos >= sampleCount )
        return 0;
    double level = controlsettings.trigger.level[ channel ];
    double timeDisplay = controlsettings.samplerate.target.duration; // time for full screen width
    double sampleRate = controlsettings.samplerate.current;
    double samplesDisplay = timeDisplay * sampleRate;

    unsigned preTrigSamples =
        startPos ? startPos : unsigned( controlsettings.trigger.position * samplesDisplay ); // samples left of trigger
    unsigned postTrigSamples =
        unsigned( sampleCount ) - ( unsigned( samplesDisplay ) - preTrigSamples ); // samples right of trigger
    // |-----------samples-----------| // available sample
    // |--disp--|                      // display size
    // |<<<<<T>>|--------------------| // >> = right = (disp-pre) i.e. right of trigger on screen
    // |<pre<|                         // << = left = pre
    // |--(samp-(disp-pre))-------|>>|
    // |<<<<<|????????????????????|>>| // ?? = search for trigger in this range [left,right]

    const unsigned swTriggerSampleSet =
        controlsettings.trigger.smooth ? 10 : 1; // check this number of samples before/after trigger point ...
    const unsigned swTriggerThreshold =
        controlsettings.trigger.smooth ? 5 : 0; // ... and get at least this number below or above trigger
    if ( postTrigSamples > sampleCount - 2 * ( swTriggerSampleSet + 1 ) )
        postTrigSamples = sampleCount - 2 * ( swTriggerSampleSet + 1 );
    // printf( "pre: %d, post %d\n", preTrigSamples, postTrigSamples );

    double prev = INT_MAX * slope;
    unsigned swTriggerStart = 0;
    for ( unsigned int i = preTrigSamples; i < postTrigSamples; i++ ) {
        if ( slope * samples[ i ] >= slope * level && slope * prev < slope * level ) { // trigger condition met
            // check for the next few SampleSet samples, if they are also above/below the trigger value
            unsigned int before = 0;
            for ( unsigned int k = i - 1; k >= i - swTriggerSampleSet && k > 0; k-- ) {
                if ( slope * samples[ k ] < slope * level )
                    before++;
            }
            unsigned int after = 0;
            for ( unsigned int k = i + 1; k <= i + swTriggerSampleSet && k < sampleCount; k++ ) {
                if ( slope * samples[ k ] >= slope * level )
                    after++;
            }
            // if at least >Threshold (=5) samples before and after trig meet the condition, set trigger
            if ( before > swTriggerThreshold && after > swTriggerThreshold ) {
                swTriggerStart = i;
                break;
            }
        }
        prev = samples[ i ];
    }
    return swTriggerStart;
}


unsigned HantekDsoControl::searchTriggerPosition() {
    static Dso::Slope nextSlope = Dso::Slope::Positive; // for alternating slope mode X
    ChannelID channel = controlsettings.trigger.source;
    // Trigger channel not in use
    if ( !controlsettings.voltage[ channel ].used || result.data.empty() ) {
        return result.triggerPosition = 0;
    }
    // printf( "HDC::searchTriggerPosition()\n" );
    triggeredPositionRaw = 0;
    result.triggerPosition = 0; // not triggered
    result.pulseWidth1 = 0.0;
    result.pulseWidth2 = 0.0;

    size_t sampleCount = result.data[ channel ].size();              // number of available samples
    double timeDisplay = controlsettings.samplerate.target.duration; // time for full screen width
    double sampleRate = controlsettings.samplerate.current;
    double samplesDisplay = timeDisplay * sampleRate;
    // unsigned preTrigSamples = (unsigned)(controlsettings.trigger.position * samplesDisplay);
    // printf( "sC %lu, tD %g, sR %g, sD %g\n", sampleCount, timeDisplay, sampleRate, samplesDisplay );
    if ( samplesDisplay >= sampleCount ) {
        // For sure not enough samples to adjust for jitter.
        qDebug() << "Too few samples to make a steady picture. Decrease sample rate";
        return result.triggerPosition = 0;
    }

    // search for trigger point in a range that leaves enough samples left and right of trigger for display
    // find also the alternate slope after trigger point -> calculate pulse width.
    if ( controlsettings.trigger.slope != Dso::Slope::Both ) {
        triggeredPositionRaw = searchTriggerPoint( nextSlope = controlsettings.trigger.slope );
        if ( triggeredPositionRaw ) { // triggered -> search also following other slope (calculate pulse width)
            if ( unsigned int slopePos2 = searchTriggerPoint( mirrorSlope( nextSlope ), triggeredPositionRaw ) ) {
                result.pulseWidth1 = ( slopePos2 - triggeredPositionRaw ) / sampleRate;
                if ( unsigned int slopePos3 = searchTriggerPoint( nextSlope, slopePos2 ) ) {
                    result.pulseWidth2 = ( slopePos3 - slopePos2 ) / sampleRate;
                }
            }
        }
    } else { // alternating trigger slope
        triggeredPositionRaw = searchTriggerPoint( nextSlope );
        if ( triggeredPositionRaw ) { // triggered -> change slope
            Dso::Slope thirdSlope = nextSlope;
            nextSlope = mirrorSlope( nextSlope );
            if ( unsigned int slopePos2 = searchTriggerPoint( nextSlope, triggeredPositionRaw ) ) {
                result.pulseWidth1 = ( slopePos2 - triggeredPositionRaw ) / sampleRate;
                if ( unsigned int slopePos3 = searchTriggerPoint( thirdSlope, slopePos2 ) ) {
                    result.pulseWidth2 = ( slopePos3 - slopePos2 ) / sampleRate;
                }
            }
        }
    }

    if ( triggeredPositionRaw ) {                      // triggered
        result.triggerPosition = triggeredPositionRaw; // align trace to trigger position
    }
    // printf( "nextSlope %c, triggerPositionRaw %d\n", "/\\"[ int( nextSlope ) ], triggeredPositionRaw );
    return result.triggerPosition;
}


bool HantekDsoControl::provideTriggeredData() {
    // printf( "HDC::provideTriggeredData()\n" );
    static DSOsamples triggeredResult; // storage for last triggered trace samples
    if ( result.triggerPosition ) {    // live trace has triggered
        // Use this trace and save it also
        triggeredResult.data = result.data;
        triggeredResult.samplerate = result.samplerate;
        triggeredResult.clipped = result.clipped;
        triggeredResult.triggerPosition = result.triggerPosition;
        result.liveTrigger = true;                                           // show green "TR" top left
    } else if ( controlsettings.trigger.mode == Dso::TriggerMode::NORMAL ) { // Not triggered in NORMAL mode
        // Use saved trace (even if it is empty)
        result.data = triggeredResult.data;
        result.samplerate = triggeredResult.samplerate;
        result.clipped = triggeredResult.clipped;
        result.triggerPosition = triggeredResult.triggerPosition;
        result.liveTrigger = false; // show red "TR" top left
    } else {                        // Not triggered and not NORMAL mode
        // Use the free running trace, discard history
        triggeredResult.data.clear();        // discard trace
        triggeredResult.triggerPosition = 0; // not triggered
        result.liveTrigger = false;          // show red "TR" top left
    }
    return result.liveTrigger;
}


/// \brief Updates the interval of the periodic thread timer.
void HantekDsoControl::updateInterval() {
    // Check the current oscilloscope state everytime 25% of the time
    //  the buffer should be refilled (-> acquireInterval in ms)
    // Use real 100% rate for demo device
    acquireInterval = int( SAMPLESIZE_USED * ( device ? 250.0 : 1000.0 ) / controlsettings.samplerate.current );
    // Slower update reduces CPU load but it worsens the triggering of rare events
    // Display can be filled at slower rate (not faster than displayInterval)
#ifdef __arm__
    // RPi: Not more often than every 10 ms but at least once every 100 ms
    acquireInterval = qBound( 10, acquireInterval, 100 );
    displayInterval = 20; // update display at least every 20 ms
#else
    // Not more often than every 1 ms but at least once every 100 ms
    acquireInterval = qBound( 1, acquireInterval, 100 );
    displayInterval = 10; // update display at least every 10 ms
#endif
}


/// \brief State machine for the device communication
void HantekDsoControl::stateMachine() {
    static int delayDisplay = 0;       // timer for display
    static bool lastTriggered = false; // state of last frame
    static bool skipEven = true;       // even or odd frames were skipped
    static uint8_t activeChannels = 2; // detect 1 <-> 2 channel change
    int errorCode = 0;
    // Send all pending control commands
    ControlCommand *controlCommand = firstControlCommand;
    while ( controlCommand ) {
        if ( controlCommand->pending ) {
            timestampDebug( QString( "Sending control command %1:%2" )
                                .arg( QString::number( controlCommand->code, 16 ),
                                      hexDump( controlCommand->data(), unsigned( controlCommand->size() ) ) ) );
            if ( controlCommand->code == uint8_t( ControlCode::CONTROL_SETNUMCHANNELS ) )
                activeChannels = *controlCommand->data();

            if ( device ) { // do the USB communication with scope HW
                errorCode = device->controlWrite( controlCommand );
                if ( errorCode < 0 ) {
                    qWarning( "Sending control command %2x failed: %s", uint8_t( controlCommand->code ),
                              libUsbErrorString( errorCode ).toLocal8Bit().data() );

                    if ( errorCode == LIBUSB_ERROR_NO_DEVICE ) {
                        emit communicationError();
                        return;
                    }
                } else {
                    controlCommand->pending = false;
                }
            } else {
                controlCommand->pending = false;
            }
        }
        controlCommand = controlCommand->next;
    }

    bool triggered = false;
    std::vector< unsigned char > rawData;
    if ( device )
        rawData = this->getSamples( expectedSampleCount );
    else
        rawData = this->getDummySamples( expectedSampleCount );
    if ( samplingStarted ) {
        convertRawDataToSamples( rawData, activeChannels ); // process new samples
        searchTriggerPosition();                            // detect trigger point
        triggered = provideTriggeredData();                 // present either free running or last triggered trace
    }
    delayDisplay += acquireInterval;
    // always run the display (slowly at t=displayInterval) to allow user interaction
    // ... but update immediately if new triggered data is available after untriggered
    // skip an even number of frames when slope == Dso::Slope::Both
    if ( ( triggered && !lastTriggered )                                 // show new data immediately
         || ( ( delayDisplay >= displayInterval )                        // or wait some time ...
              && ( ( controlsettings.trigger.slope != Dso::Slope::Both ) // ... for ↗ or ↘ slope
                   || skipEven ) ) )                                     // and drop even no. of frames
    {
        delayDisplay = 0;
        skipEven = true; // zero frames -> even
        emit samplesAvailable( &result );
    } else {
        skipEven = !skipEven;
    }

    lastTriggered = triggered; // save state

    // Stop sampling if we're in single trigger mode and have a triggered trace (txh No13)
    if ( controlsettings.trigger.mode == Dso::TriggerMode::SINGLE && samplingStarted && triggeredPositionRaw > 0 ) {
        enableSampling( false );
    }

    // Sampling completed, restart it when necessary
    samplingStarted = false;

    if ( sampling ) {
        // Sampling hasn't started, update the expected sample count
        expectedSampleCount = this->getSampleCount();
        timestampDebug( "Starting to capture" );
        samplingStarted = true;
    }
    updateInterval(); // calculate new acquire timing
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 4, 0 ) )
    QTimer::singleShot( acquireInterval, this, &HantekDsoControl::stateMachine );
#else
    QTimer::singleShot( acquireInterval, this, SLOT( stateMachine() ) );
#endif
}


void HantekDsoControl::addCommand( ControlCommand *newCommand, bool pending ) {
    newCommand->pending = pending;
    control[ newCommand->code ] = newCommand;
    newCommand->next = firstControlCommand;
    firstControlCommand = newCommand;
}


Dso::ErrorCode HantekDsoControl::stringCommand( const QString &commandString ) {
    if ( !deviceIsConnected() )
        return Dso::ErrorCode::CONNECTION;

    QStringList commandParts = commandString.split( ' ', QString::SkipEmptyParts );

    if ( commandParts.count() < 1 )
        return Dso::ErrorCode::PARAMETER;
    if ( commandParts[ 0 ] != "send" )
        return Dso::ErrorCode::UNSUPPORTED;
    if ( commandParts.count() < 2 )
        return Dso::ErrorCode::PARAMETER;

    uint8_t codeIndex = 0;
    hexParse( commandParts[ 2 ], &codeIndex, 1 );
    QString data = commandString.section( ' ', 2, -1, QString::SectionSkipEmpty );

    if ( commandParts[ 1 ] == "control" ) {
        if ( !control[ codeIndex ] )
            return Dso::ErrorCode::UNSUPPORTED;

        ControlCommand *c = modifyCommand< ControlCommand >( ControlCode( codeIndex ) );
        hexParse( data, c->data(), unsigned( c->size() ) );
        return Dso::ErrorCode::NONE;
    } else
        return Dso::ErrorCode::UNSUPPORTED;
}
