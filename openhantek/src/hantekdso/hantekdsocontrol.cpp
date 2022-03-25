// SPDX-License-Identifier: GPL-2.0+

// #define TIMESTAMPDEBUG

#include <assert.h>
#include <cmath>
#include <limits>
#include <vector>

#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QSettings>
#include <QTimer>
#include <QtCore>

#include <stdio.h>

#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "scopesettings.h"
#include "usb/scopedevice.h"

using namespace Hantek;
using namespace Dso;


HantekDsoControl::HantekDsoControl( ScopeDevice *device, const DSOModel *model, unsigned verboseLevel )
    : verboseLevel( verboseLevel ), scopeDevice( device ), model( model ), specification( model->spec() ),
      controlsettings( &( specification->samplerate.single ), specification->channels ) {
    qRegisterMetaType< DSOsamples * >();
    if ( verboseLevel > 1 )
        qDebug() << " HantekDsoControl::HantekDsoControl()";

    if ( device && specification->fixedUSBinLength )
        device->overwriteInPacketLength( unsigned( specification->fixedUSBinLength ) );
    // Apply special requirements by the devices model
    model->applyRequirements( this );

    getCalibrationValues();

    stateMachineRunning = true;
}


HantekDsoControl::~HantekDsoControl() {
    if ( scope->verboseLevel > 1 )
        qDebug() << " HantekDsoControl::~HantekDsoControl()";
    while ( firstControlCommand ) {
        ControlCommand *t = firstControlCommand->next;
        delete firstControlCommand;
        firstControlCommand = t;
    }
}


void HantekDsoControl::prepareForShutdown() {
    if ( verboseLevel > 1 )
        qDebug() << " HDC::prepareForShutdown()";
    calibrateOffset( false );
    updateCalibrationValues( scopeDevice->isRealHW() && specification->hasCalibrationEEPROM );
}


bool HantekDsoControl::deviceNotConnected() { return !scopeDevice->isConnected(); }


void HantekDsoControl::restoreTargets() {
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::restoreTargets()";
    if ( controlsettings.samplerate.target.samplerateSet == ControlSettingsSamplerateTarget::Samplerrate )
        setSamplerate();
    else
        setRecordTime();
}


void HantekDsoControl::updateSamplerateLimits() {
    QList< double > sampleSteps;
    double limit = isSingleChannel() ? specification->samplerate.single.max : specification->samplerate.multi.max;

    if ( controlsettings.samplerate.current > limit ) {
        setSamplerate( limit );
    }
    for ( auto &v : specification->fixedSampleRates ) {
        if ( v.samplerate <= limit ) {
            sampleSteps << v.samplerate;
        }
    }
    if ( verboseLevel > 3 )
        qDebug() << "   HDC::updateSamplerateLimits()" << sampleSteps;
    else if ( verboseLevel > 2 )
        qDebug() << "  HDC::updateSamplerateLimits()" << sampleSteps.first() << "..." << sampleSteps.last();
    emit samplerateSet( 1, sampleSteps );
}


void HantekDsoControl::controlSetSamplerate( uint8_t sampleIndex ) {
    static uint8_t lastIndex = 0xFF;
    uint8_t id = specification->fixedSampleRates[ sampleIndex ].id;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::controlSetSamplerate()" << sampleIndex << "id:" << id;
    modifyCommand< ControlSetSamplerate >( ControlCode::CONTROL_SETSAMPLERATE )->setSamplerate( id, sampleIndex );
    if ( sampleIndex != lastIndex ) { // samplerate has changed, start new sampling
        restartSampling();
    }
    lastIndex = sampleIndex;
}


Dso::ErrorCode HantekDsoControl::setSamplerate( double samplerate ) {
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setSamplerate()" << samplerate;
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( samplerate == 0.0 ) {
        samplerate = controlsettings.samplerate.target.samplerate;
    } else {
        controlsettings.samplerate.target.samplerate = samplerate;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Samplerrate;
    }
    uint8_t sampleIndex;
    for ( sampleIndex = 0; sampleIndex < specification->fixedSampleRates.size() - 1; ++sampleIndex ) {
        if ( long( round( specification->fixedSampleRates[ sampleIndex ].samplerate ) ) ==
             long( round( samplerate ) ) ) // dont compare double == double
            break;
    }
    controlSetSamplerate( sampleIndex );
    setDownsampling( specification->fixedSampleRates[ sampleIndex ].oversampling );
    controlsettings.samplerate.current = samplerate;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setSamplerate() emit samplerateChanged" << samplerate;
    emit samplerateChanged( samplerate );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setRecordTime( double duration ) {
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setRecordTime()" << duration;
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( duration == 0.0 ) {
        duration = controlsettings.samplerate.target.duration;
        return Dso::ErrorCode::NONE;
    } else {
        controlsettings.samplerate.target.duration = duration;
        controlsettings.samplerate.target.samplerateSet = ControlSettingsSamplerateTarget::Duration;
    }
    if ( verboseLevel > 2 )
        qDebug() << "  duration =" << duration;

    double srLimit;
    if ( isSingleChannel() )
        srLimit = ( specification->samplerate.single ).max;
    else
        srLimit = ( specification->samplerate.multi ).max;
    // For now - we go for the SAMPLESIZE (= 20000) size sampling, defined in hantekdsocontrol.h
    // Find highest samplerate using less equal half of these samples to obtain our duration.
    uint8_t sampleIndex = 0;
    for ( uint8_t iii = 0; iii < specification->fixedSampleRates.size(); ++iii ) {
        double sRate = specification->fixedSampleRates[ iii ].samplerate;
        if ( verboseLevel > 3 )
            qDebug() << "   sampleIndex:" << sampleIndex << "sRate:" << sRate << "sRate*duration:" << sRate * duration;
        // Ensure that at least 1/2 of remaining samples are available for SW trigger algorithm
        // for stability reason avoid the highest sample rate as default
        if ( sRate < srLimit && sRate * duration <= SAMPLESIZE / 2 ) {
            sampleIndex = iii;
        }
    }
    controlSetSamplerate( sampleIndex );
    setDownsampling( specification->fixedSampleRates[ sampleIndex ].oversampling );
    double samplerate = specification->fixedSampleRates[ sampleIndex ].samplerate;
    controlsettings.samplerate.current = samplerate;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setRecordTime() emit samplerateChanged" << samplerate;
    emit samplerateChanged( samplerate );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCalFreq( double calfreq ) {
    unsigned int cf;
    if ( calfreq < 1000 ) { // 50, 60, 100, 200, 500 -> 105, 106, 110, 120, 150
        cf = 100 + unsigned( round( calfreq / 10 ) );
        if ( 110 == cf ) // special case for sigrok FW (e.g. DDS120) 100Hz -> 0
            cf = 0;
    } else if ( calfreq <= 5500 && unsigned( calfreq ) % 1000 ) { // non integer multiples of 1 kHz
        cf = 200 + unsigned( round( calfreq / 100 ) );            // in the range 1000 .. 5500 Hz
    } else {
        cf = unsigned( round( calfreq / 1000 ) ); // 1000, ..., 100000 -> 1, ..., 100
    }
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setCalFreq()" << calfreq << cf;
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    // control command for setting
    modifyCommand< ControlSetCalFreq >( ControlCode::CONTROL_SETCALFREQ )->setCalFreq( uint8_t( cf ) );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setChannelUsed( ChannelID channel, bool used ) {
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setChannelUsed()" << channel << used;
    if ( deviceNotConnected() )
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
    setSingleChannel( usedChannels == UsedChannels::USED_CH1 );
    if ( verboseLevel > 2 )
        qDebug() << "  usedChannels" << QString( "%1" ).arg( int( usedChannels ), 2, 2, QLatin1Char( '0' ) );
    modifyCommand< ControlSetNumChannels >( ControlCode::CONTROL_SETNUMCHANNELS )->setNumChannels( isSingleChannel() ? 1 : 2 );
    // Check if fast rate mode availability changed
    updateSamplerateLimits();
    restoreTargets();
    // sampleSetupChanged = true; // skip next raw samples block to avoid artefacts
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setChannelInverted( ChannelID channel, bool inverted ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    // Update settings
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setChannelInverted()" << channel << inverted;
    controlsettings.voltage[ channel ].inverted = inverted;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setGain( ChannelID channel, double gain ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;

    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setGain()" << channel << gain;
    static uint8_t lastGain[ 2 ] = { 0xFF, 0xFF };
    gain /= controlsettings.voltage[ channel ].probeAttn; // gain needs to be scaled by probe attenuation
    // Find lowest gain voltage thats at least as high as the requested
    uint8_t gainID;
    for ( gainID = 0; gainID < specification->gain.size() - 1; ++gainID )
        if ( specification->gain[ gainID ].Vdiv >= gain )
            break;
    uint8_t gainValue = specification->gain[ gainID ].gainValue;
    if ( channel == 0 ) {
        modifyCommand< ControlSetGain_CH1 >( ControlCode::CONTROL_SETGAIN_CH1 )->setGainCH1( gainValue, gainID );
        if ( lastGain[ 0 ] != gainValue ) { // HW gain changed, start new samples
            restartSampling();
        }
        lastGain[ 0 ] = gainValue;
    } else if ( channel == 1 ) {
        modifyCommand< ControlSetGain_CH2 >( ControlCode::CONTROL_SETGAIN_CH2 )->setGainCH2( gainValue, gainID );
        if ( lastGain[ 1 ] != gainValue ) { // HW gain changed, start new samples
            restartSampling();
        }
        lastGain[ 1 ] = gainValue;
    } else
        qDebug( "%s: Unsupported channel: %i\n", __func__, channel );
    controlsettings.voltage[ channel ].gain = gainID;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setProbe( ChannelID channel, double probeAttn ) {
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;

    controlsettings.voltage[ channel ].probeAttn = probeAttn;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setProbe()" << channel << probeAttn;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setCoupling( ChannelID channel, Dso::Coupling coupling ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;

    static int lastCoupling[ 2 ] = { -1, -1 };
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setCoupling()" << channel << int( coupling );
    if ( hasCommand( ControlCode::CONTROL_SETCOUPLING ) ) // don't send command if it is not implemented (like on the 6022)
        modifyCommand< ControlSetCoupling >( ControlCode::CONTROL_SETCOUPLING )
            ->setCoupling( channel, coupling == Dso::Coupling::DC );
    controlsettings.voltage[ channel ].coupling = coupling;
    if ( lastCoupling[ channel ] != int( coupling ) ) { // HW coupling changed, start new samples
        restartSampling();
    }
    lastCoupling[ channel ] = int( coupling );
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerMode( Dso::TriggerMode mode ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;

    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerMode()" << int( mode );
    static Dso::TriggerMode lastMode;
    controlsettings.trigger.mode = mode;
    if ( Dso::TriggerMode::SINGLE != mode )
        enableSampling( true );
    // trigger mode changed NONE <-> !NONE
    if ( ( Dso::TriggerMode::ROLL == mode && Dso::TriggerMode::ROLL != lastMode ) ||
         ( Dso::TriggerMode::ROLL != mode && Dso::TriggerMode::ROLL == lastMode ) ) {
        restartSampling(); // invalidate old samples
        raw.freeRun = Dso::TriggerMode::ROLL == mode;
    }
    lastMode = mode;
    newTriggerParam = true;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSource( int channel ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerSource()" << channel;
    controlsettings.trigger.source = channel;
    newTriggerParam = true;
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSmooth( int smooth ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerSmooth()" << smooth;
    controlsettings.trigger.smooth = smooth;
    newTriggerParam = true;
    return Dso::ErrorCode::NONE;
}


// trigger level in Volt
Dso::ErrorCode HantekDsoControl::setTriggerLevel( ChannelID channel, double level ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( channel >= specification->channels )
        return Dso::ErrorCode::PARAMETER;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerLevel()" << channel << level;
    controlsettings.trigger.level[ channel ] = level;
    newTriggerParam = true;
    displayInterval = 0; // update screen immediately
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::setTriggerSlope( Dso::Slope slope ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerSlope()" << int( slope );
    controlsettings.trigger.slope = slope;
    newTriggerParam = true;
    return Dso::ErrorCode::NONE;
}


// set trigger position (0.0 - 1.0)
Dso::ErrorCode HantekDsoControl::setTriggerPosition( double position ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::setTriggerPosition()" << position;
    controlsettings.trigger.position = position;
    newTriggerParam = true;
    return Dso::ErrorCode::NONE;
}


// Initialize the device with the current settings.
void HantekDsoControl::applySettings( DsoSettingsScope *dsoSettingsScope ) {
    if ( verboseLevel > 1 )
        qDebug() << " HDC::applySettings()";
    scope = dsoSettingsScope;
    bool mathUsed = dsoSettingsScope->anyUsed( specification->channels );
    for ( ChannelID channel = 0; channel < specification->channels; ++channel ) {
        setProbe( channel, dsoSettingsScope->voltage[ channel ].probeAttn );
        setGain( channel, dsoSettingsScope->gain( channel ) );
        setTriggerLevel( channel, dsoSettingsScope->voltage[ channel ].trigger );
        setChannelUsed( channel, mathUsed | dsoSettingsScope->anyUsed( channel ) );
        setChannelInverted( channel, dsoSettingsScope->voltage[ channel ].inverted );
        setCoupling( channel, Dso::Coupling( dsoSettingsScope->voltage[ channel ].couplingOrMathIndex ) );
    }

    setRecordTime( dsoSettingsScope->horizontal.timebase * DIVS_TIME );
    setCalFreq( dsoSettingsScope->horizontal.calfreq );
    setTriggerMode( dsoSettingsScope->trigger.mode );
    setTriggerPosition( dsoSettingsScope->trigger.position );
    setTriggerSlope( dsoSettingsScope->trigger.slope );
    setTriggerSource( dsoSettingsScope->trigger.source );
    setTriggerSmooth( dsoSettingsScope->trigger.smooth );
}


/// \brief Starts a new sampling block.
void HantekDsoControl::restartSampling() {
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::restartSampling()";
    scopeDevice->stopSampling();
    raw.rollMode = false;
}


/// \brief Start sampling process.
void HantekDsoControl::enableSampling( bool enabled ) {
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::enableSampling()" << enabled;
    sampling = enabled;
    updateSamplerateLimits();
    emit samplingStatusChanged( enabled );
}


unsigned HantekDsoControl::getRecordLength() const {
    unsigned rawsize = getSamplesize();
    rawsize *= downsamplingNumber;         // take multiple samples for oversampling
    rawsize = grossSampleCount( rawsize ); // adjust for skipping of minimal 2048 leading samples
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::getRecordLength() ->" << rawsize;
    return rawsize;
}


Dso::ErrorCode HantekDsoControl::getCalibrationValues() {
    // Persistent storage: unique offset/gain calibration file:
    // Linux, Unix, macOS: "$HOME/.config/OpenHantek/DSO-6022BE_NNNNNNNNNNNN_calibration.ini"
    // Windows: "%APPDATA%\OpenHantek\DSO-6022BE_NNNNNNNNNNNN_calibration.ini"
    QString calName = scopeDevice->getModel()->name + "_" + scopeDevice->getSerialNumber() + "_calibration";
    if ( verboseLevel > 2 )
        qDebug() << "  Calibration data:" << calName + ".ini";

    calibrationSettings = std::unique_ptr< QSettings >(
        new QSettings( QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), calName ) );

    // load the offsets (persistent, saved at shutdown as "*.ini" file,  )
    calibrationSettings->beginGroup( "offset" );
    for ( int ch = 0; ch < HANTEK_CHANNEL_NUMBER; ++ch ) {
        calibrationSettings->beginGroup( "ch" + QString::number( ch ) );
        int index = 0;
        for ( const auto &g : model->spec()->gain ) {
            offsetCorrection[ index ][ ch ] =
                calibrationSettings->value( ( QString::number( int( g.Vdiv * 1000 ) ) + "mV" ), 0.0 ).toDouble();
            ++index;
        }
        calibrationSettings->endGroup();
    }
    calibrationSettings->endGroup();

    // load the gain (provided by user)
    calibrationSettings->beginGroup( "gain" );
    for ( int ch = 0; ch < 2; ++ch ) {
        calibrationSettings->beginGroup( "ch" + QString::number( ch ) );
        int index = 0;
        for ( const auto &g : model->spec()->gain ) {
            gainCorrection[ index ][ ch ] =
                calibrationSettings->value( ( QString::number( int( g.Vdiv * 1000 ) ) + "mV" ), 1.0 ).toDouble();
            ++index;
        }
        calibrationSettings->endGroup();
    }
    calibrationSettings->endGroup(); // gain

    calibrationSettings->beginGroup( "eeprom" );
    replaceCalibrationEEPROM = calibrationSettings->value( "replace_eeprom", false ).toBool();
    calibrationSettings->endGroup(); // eeprom

    if ( replaceCalibrationEEPROM ) // values created by python tool "calibrate_6022.py" replace the EEPROM content
        memset( controlsettings.cmdGetCalibration.data(), 0xFF, sizeof( CalibrationValues ) );
    else // enhance the intrinsic calibration values from EEPROM
        getCalibrationFromEEPROM();

    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::updateCalibrationValues( bool useEEPROM ) {
    if ( calibrationHasChanged ) {
        if ( verboseLevel > 2 )
            qDebug() << "  Write calibration data into" << ( useEEPROM ? "EEPROM" : "iniFile" );

        calibrationSettings->beginGroup( "gain" );
        for ( int ch = 0; ch < HANTEK_CHANNEL_NUMBER; ++ch ) {
            calibrationSettings->beginGroup( "ch" + QString::number( ch ) );
            int index = 0;
            double gain = 1.0;
            for ( const auto &g : model->spec()->gain ) {
                if ( !useEEPROM )
                    gain = round( 100.0 * gainCorrection[ index ][ ch ] ) / 100.0;
                calibrationSettings->setValue( QString::number( int( g.Vdiv * 1000 ) ) + "mV", gain );
                // qDebug() << QString::number( int( g.Vdiv * 1000 ) ) + "mV" << gain;
                ++index;
            }
            calibrationSettings->endGroup();
        }
        calibrationSettings->endGroup();

        calibrationSettings->beginGroup( "offset" );
        for ( int ch = 0; ch < HANTEK_CHANNEL_NUMBER; ++ch ) {
            calibrationSettings->beginGroup( "ch" + QString::number( ch ) );
            int index = 0;
            double offset = 0.0;
            for ( const auto &g : model->spec()->gain ) {
                if ( !useEEPROM )
                    offset = round( 100.0 * offsetCorrection[ index ][ ch ] ) / 100.0;
                calibrationSettings->setValue( QString::number( int( g.Vdiv * 1000 ) ) + "mV", offset );
                // qDebug() << QString::number( int( g.Vdiv * 1000 ) ) + "mV" << offset;
                ++index;
            }
            calibrationSettings->endGroup();
        }
        calibrationSettings->endGroup();

        calibrationSettings->beginGroup( "eeprom" );
        calibrationSettings->setValue( "replace_eeprom", !useEEPROM );
        calibrationSettings->endGroup(); // eeprom

        if ( useEEPROM )
            writeCalibrationToEEPROM();
    }
    return Dso::ErrorCode::NONE;
}


Dso::ErrorCode HantekDsoControl::getCalibrationFromEEPROM() {
    // Get calibration data from EEPROM
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::getCalibrationFromEEPROM()";
    int errorCode = -1;
    if ( scopeDevice->isRealHW() && specification->hasCalibrationEEPROM )
        errorCode = scopeDevice->controlRead( &controlsettings.cmdGetCalibration );
    if ( errorCode < 0 ) {
        // invalidate the calibration values.
        memset( controlsettings.calibrationValues, 0xFF, sizeof( CalibrationValues ) );
        if ( scopeDevice->isRealHW() ) {
            QString message = tr( "Couldn't get calibration data from oscilloscope's EEPROM. Use a config file for calibration!" );
            qWarning() << message;
            emit statusMessage( message, 0 );
            emit communicationError();
            return Dso::ErrorCode::CONNECTION;
        } else {
            return Dso::ErrorCode::NONE;
        }
    }
    memcpy( controlsettings.calibrationValues, controlsettings.cmdGetCalibration.data(), sizeof( CalibrationValues ) );
    if ( verboseLevel > 3 ) {
        QDebug line = qDebug().noquote();
        line << "   HDC::calibrationValues" << sizeof( CalibrationValues );
        line = qDebug().noquote() << "   .off.ls: ";
        for ( int g = 0; g < 8; ++g )
            for ( int c = 0; c < 2; ++c )
                line << QString::number( controlsettings.calibrationValues->off.ls.step[ g ][ c ], 16 );
        line = qDebug().noquote() << "   .off.hs: ";
        for ( int g = 0; g < 8; ++g )
            for ( int c = 0; c < 2; ++c )
                line << QString::number( controlsettings.calibrationValues->off.hs.step[ g ][ c ], 16 );
        line = qDebug().noquote() << "   .gain:   ";
        for ( int g = 0; g < 8; ++g )
            for ( int c = 0; c < 2; ++c )
                line << QString::number( controlsettings.calibrationValues->gain.step[ g ][ c ], 16 );
        line = qDebug().noquote() << "   .fine.ls:";
        for ( int g = 0; g < 8; ++g )
            for ( int c = 0; c < 2; ++c )
                line << QString::number( controlsettings.calibrationValues->fine.ls.step[ g ][ c ], 16 );
        line = qDebug().noquote() << "   .fine.hs:";
        for ( int g = 0; g < 8; ++g )
            for ( int c = 0; c < 2; ++c )
                line << QString::number( controlsettings.calibrationValues->fine.hs.step[ g ][ c ], 16 );
    }
    return Dso::ErrorCode::NONE;
}


#define TRANS_TYPE_READ 0xc0
#define TRANS_TYPE_WRITE 0x40
#define EEPROM 0xa2

Dso::ErrorCode HantekDsoControl::writeCalibrationToEEPROM() {
    uint8_t type = TRANS_TYPE_WRITE;
    uint8_t request = EEPROM;

    int value = 8;
    int index = 0;
    typedef uint8_t *uint8_p;

    // save raw offset values
    int ret = scopeDevice->controlTransfer( type, request, uint8_p( &controlsettings.correctionValues->off ),
                                            sizeof( CalibrationValues::off ), value, index );
    if ( ret < 0 ) {
        fprintf( stderr, "Unable to control transfer\n" );
        perror( "libusb_control_transfer" );
        return Dso::ErrorCode::CONNECTION;
    }
    // save gain values
    value += int( sizeof( CalibrationValues::off ) );
    ret = scopeDevice->controlTransfer( type, request, uint8_p( &controlsettings.correctionValues->gain ),
                                        sizeof( CalibrationValues::gain ), value, index );
    if ( ret < 0 ) {
        fprintf( stderr, "Unable to control transfer\n" );
        perror( "libusb_control_transfer" );
        return Dso::ErrorCode::CONNECTION;
    }

    // save fine offset values
    value += int( sizeof( CalibrationValues::gain ) );
    ret = scopeDevice->controlTransfer( type, request, uint8_p( &controlsettings.correctionValues->fine ),
                                        sizeof( CalibrationValues::fine ), value, index );
    if ( ret < 0 ) {
        fprintf( stderr, "Unable to control transfer\n" );
        perror( "libusb_control_transfer" );
        return Dso::ErrorCode::CONNECTION;
    }

    return Dso::ErrorCode::NONE;
}


void HantekDsoControl::calibrateOffset( bool enable ) {
    if ( enable ) {
        if ( !scope->liveCalibrationActive )
            memcpy( controlsettings.correctionValues, controlsettings.calibrationValues, sizeof( CalibrationValues ) );
    } else {
        if ( scope->liveCalibrationActive )
            calibrationHasChanged = true;
    }
}


void HantekDsoControl::quitSampling() {
    if ( verboseLevel > 2 )
        qDebug() << "  HDC::quitSampling()";
    enableSampling( false );
    capturing = false;
    scopeDevice->stopSampling();
    if ( scopeDevice->isDemoDevice() )
        return;
    auto controlCommand = ControlStopSampling();
    timestampDebug( QString( "Sending control command 0x%1 (Stop Sampling): %2" )
                        .arg( QString::number( controlCommand.code, 16 ),
                              hexdecDump( controlCommand.data(), unsigned( controlCommand.size() ) ) ) );
    int errorCode = scopeDevice->controlWrite( &controlCommand );
    if ( errorCode < 0 ) {
        qWarning() << "controlWrite: stop sampling failed: " << libUsbErrorString( errorCode );
        emit communicationError();
    }
}


static double byteToGain( uint8_t gain ) {
    if ( gain && gain != 255 ) // data valid
        return 1.0 + ( gain - 0x80 ) / 500.0;
    else
        return 1.0;
}


static uint8_t gainToByte( double gain ) {
    if ( gain >= 0.75 && gain <= 1.25 )
        return uint8_t( round( 0x80 + 500 * ( gain - 1 ) ) );
    else
        return 0;
}


static double bytesToOffset( uint8_t offsetRaw, uint8_t offsetFine ) {
    if ( offsetRaw && offsetRaw != 255 && offsetFine && offsetFine != 255 ) { // data valid
        return offsetRaw + ( offsetFine - 0x80 ) / 250.0;
    } else {         // no offset correction
        return 0x80; // ADC has "binary offset" format (0x80 = 0V)
    }
}


static uint8_t offsetToRaw( double offset ) {
    if ( offset > 96 && offset < 140 )
        return uint8_t( round( offset ) );
    else
        return 0;
}


static uint8_t offsetToFine( double offset ) {
    if ( offset > 96 && offset < 140 ) {
        offset -= round( offset );
        return uint8_t( round( 0x80 + 250 * offset ) );
    } else
        return 0;
}


void HantekDsoControl::convertRawDataToSamples() {
    QReadLocker rawLocker( &raw.lock );
    activeChannels = raw.channels;
    const unsigned rawSampleCount = unsigned( raw.data.size() ) / activeChannels;
    if ( !rawSampleCount )
        return;
    const unsigned rawOversampling = raw.oversampling;
    const bool freeRunning = rawSampleCount / rawOversampling < SAMPLESIZE; // amount needed for sw trigger
    const unsigned sampleCount = freeRunning ? rawSampleCount : netSampleCount( rawSampleCount );
    const unsigned resultSamples = freeRunning ? sampleCount / rawOversampling - 1 : sampleCount / rawOversampling;
    const unsigned skipSamples = rawSampleCount - sampleCount;
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::convertRawDataToSamples()" << raw.tag;
    QWriteLocker resultLocker( &result.lock );
    result.freeRunning = freeRunning;
    result.tag = raw.tag;
    result.samplerate = raw.samplerate / raw.oversampling;
    // Prepare result buffers
    result.data.resize( specification->channels );
    for ( ChannelID channelCounter = 0; channelCounter < specification->channels; ++channelCounter )
        result.data[ channelCounter ].clear();

    // Convert channel data
    // Channels are using their separate buffers
    for ( ChannelID channel = 0; channel < activeChannels; ++channel ) {
        const unsigned gainIndex = raw.gainIndex[ channel ];
        const double voltageScale = specification->voltageScale[ channel ][ gainIndex ];
        const double probeAttn = controlsettings.voltage[ channel ].probeAttn;
        const double sign = controlsettings.voltage[ channel ].inverted ? -1.0 : 1.0;

        // shift + individual offset for each channel and gain
        // get offset value from eeprom[ 8 .. 39 and (if available) 56 .. 87]
        uint8_t offsetRaw;
        uint8_t offsetFine;
        if ( result.samplerate < 30e6 ) {
            offsetRaw = controlsettings.calibrationValues->off.ls.step[ gainIndex ][ channel ];
            offsetFine = controlsettings.calibrationValues->fine.ls.step[ gainIndex ][ channel ];
        } else {
            offsetRaw = controlsettings.calibrationValues->off.hs.step[ gainIndex ][ channel ];
            offsetFine = controlsettings.calibrationValues->fine.hs.step[ gainIndex ][ channel ];
        }
        // calibration values from EEPROM
        double offsetCalibration = bytesToOffset( offsetRaw, offsetFine );
        double gainCalibration = byteToGain( controlsettings.calibrationValues->gain.step[ gainIndex ][ channel ] );
        // Convert data from the oscilloscope and write it into the channel sample buffer
        unsigned rawBufPos = 0;
        if ( raw.freeRun && raw.rollMode ) // show the "new" samples on the right screen side
            rawBufPos = raw.received;      // start with remaining "old" samples in buffer
        result.data[ channel ].resize( resultSamples );
        rawBufPos += skipSamples * activeChannels; // skip first unstable samples
        result.clipped &= ~( 0x01 << channel );    // clear clipping flag

        double gainCorr = gainCorrection[ gainIndex ][ channel ];
        double offsetCorr = offsetCorrection[ gainIndex ][ channel ];
        double liveOffset = 0.0;

        int minValue = 0xFF;
        int maxValue = 0x00;

        for ( unsigned index = 0; index < resultSamples;
              ++index, rawBufPos += activeChannels * rawOversampling ) { // advance either by one or two blocks
            if ( rawBufPos + rawOversampling * activeChannels > rawSampleCount * activeChannels )
                rawBufPos = 0; // (roll mode) show "new" samples after the "old" samples
            double sample = 0.0;
            for ( unsigned iii = 0; iii < rawOversampling * activeChannels; iii += activeChannels ) {
                int rawSample = raw.data[ rawBufPos + channel + iii ]; // CH1/CH2/CH1/CH2 ...
                if ( rawSample == 0x00 || rawSample == 0xFF )          // min or max -> clipped
                    result.clipped |= 0x01 << channel;
                if ( rawSample > maxValue )
                    maxValue = rawSample;
                if ( rawSample < minValue )
                    minValue = rawSample;
                sample += double( rawSample ) - offsetCalibration;
            }
            sample /= rawOversampling;
            if ( scope->liveCalibrationActive ) {
                liveOffset += sample;
            }
            // qDebug() << channel << offsetCorrection[ gainIndex ][ channel ];
            sample -= offsetCorr;
            sample *= gainCorr;

            result.data[ channel ][ index ] = sign * sample / voltageScale * gainCalibration * probeAttn;
        }
        liveOffset /= resultSamples;

        if ( maxValue - minValue > 10 || liveOffset > 20 ) { // big jitter/noise, offset too big
            emit liveCalibrationError();                     // stop live calibration without storing something
        }

        if ( scope->liveCalibrationActive ) {
            offsetCorrection[ gainIndex ][ channel ] = liveOffset;
            if ( result.samplerate < 30e6 ) {
                controlsettings.correctionValues->off.ls.step[ gainIndex ][ channel ] =
                    offsetToRaw( liveOffset + offsetCalibration );
                controlsettings.correctionValues->fine.ls.step[ gainIndex ][ channel ] =
                    offsetToFine( liveOffset + offsetCalibration );
            } else {
                controlsettings.correctionValues->off.hs.step[ gainIndex ][ channel ] =
                    offsetToRaw( liveOffset + offsetCalibration );
                controlsettings.correctionValues->fine.hs.step[ gainIndex ][ channel ] =
                    offsetToFine( liveOffset + offsetCalibration );
            }
            controlsettings.correctionValues->gain.step[ gainIndex ][ channel ] = gainToByte( gainCorr * gainCalibration );
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

    unsigned channel = unsigned( controlsettings.trigger.source );
    const std::vector< double > &samples = result.data[ channel ];
    unsigned sampleCount = unsigned( samples.size() ); ///< number of available samples
    if ( verboseLevel > 5 )
        qDebug() << "     HDC::searchTriggerPoint()" << int( dsoSlope ) << startPos;
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

    const unsigned swTriggerSampleSet = unsigned( pow( 20, controlsettings.trigger.smooth ) );
    if ( postTrigSamples > sampleCount - 2 * ( swTriggerSampleSet + 1 ) )
        postTrigSamples = sampleCount - 2 * ( swTriggerSampleSet + 1 );
    if ( verboseLevel > 5 )
        qDebug() << "     pre:" << preTrigSamples << "post:" << postTrigSamples;

    double prev = INT_MAX;
    unsigned swTriggerStart = 0;
    for ( unsigned int i = preTrigSamples; i < postTrigSamples; i++ ) {
        if ( slope * samples[ i ] >= slope * level && slope * prev < slope * level ) { // trigger condition met
            // check for the previous few SampleSet samples, if they are also above/below the trigger value
            bool triggerBefore = false;
            double mean = 0;
            unsigned iii = 0;
            for ( int k = (int)(i) - 1; k >= (int)(i) - (int)(swTriggerSampleSet) && k > 0; k-- ) {
                mean += samples[ k ];
                iii++;
            }
            if ( iii ) {
                mean /= iii;
                triggerBefore = slope * mean < slope * level;
            }
            // check for the next few SampleSet samples, if they are also above/below the trigger value
            bool triggerAfter = false;
            mean = 0;
            iii = 0;
            for ( unsigned int k = i + 1; k <= i + swTriggerSampleSet && k < sampleCount; k++ ) {
                mean += samples[ k ];
                iii++;
            }
            if ( iii ) {
                mean /= iii;
                triggerAfter = slope * mean > slope * level;
            }
            // if at least >Threshold samples before and after trig meet the condition, set trigger
            if ( triggerBefore && triggerAfter ) {
                swTriggerStart = i;
                break;
            }
        }
        prev = samples[ i ];
    }
    return swTriggerStart;
}


unsigned HantekDsoControl::searchTriggeredPosition() {
    static Dso::Slope nextSlope = Dso::Slope::Positive; // for alternating slope mode X
    ChannelID channel = ChannelID( controlsettings.trigger.source );
    // Trigger channel not in use
    if ( !controlsettings.voltage[ channel ].used || result.data.empty() ) {
        return result.triggeredPosition = 0;
    }
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::searchTriggeredPosition()" << result.tag;
    triggeredPositionRaw = 0;
    double pulseWidth1 = 0.0;
    double pulseWidth2 = 0.0;

    size_t sampleCount = result.data[ channel ].size();              // number of available samples
    double timeDisplay = controlsettings.samplerate.target.duration; // time for full screen width
    double sampleRate = result.samplerate;                           //
    unsigned samplesDisplay = unsigned( round( timeDisplay * controlsettings.samplerate.current ) );
    if ( sampleCount < samplesDisplay ) // not enough samples to adjust for jitter.
        return result.triggeredPosition = 0;
    // search for trigger point in a range that leaves enough samples left and right of trigger for display
    // find also the alternate slope after trigger point -> calculate pulse width.
    if ( controlsettings.trigger.slope != Dso::Slope::Both ) {
        triggeredPositionRaw = searchTriggerPoint( nextSlope = controlsettings.trigger.slope );
        if ( triggeredPositionRaw ) { // triggered -> search also following other slope (calculate pulse width)
            if ( unsigned int slopePos2 = searchTriggerPoint( mirrorSlope( nextSlope ), triggeredPositionRaw ) ) {
                pulseWidth1 = ( slopePos2 - triggeredPositionRaw ) / sampleRate;
                if ( unsigned int slopePos3 = searchTriggerPoint( nextSlope, slopePos2 ) ) {
                    pulseWidth2 = ( slopePos3 - slopePos2 ) / sampleRate;
                }
            }
        }
    } else { // alternating trigger slope
        triggeredPositionRaw = searchTriggerPoint( nextSlope );
        if ( triggeredPositionRaw ) { // triggered -> change slope
            Dso::Slope thirdSlope = nextSlope;
            nextSlope = mirrorSlope( nextSlope );
            if ( unsigned int slopePos2 = searchTriggerPoint( nextSlope, triggeredPositionRaw ) ) {
                pulseWidth1 = ( slopePos2 - triggeredPositionRaw ) / sampleRate;
                if ( unsigned int slopePos3 = searchTriggerPoint( thirdSlope, slopePos2 ) ) {
                    pulseWidth2 = ( slopePos3 - slopePos2 ) / sampleRate;
                }
            }
        }
    }

    result.triggeredPosition = triggeredPositionRaw; // align trace to trigger position
    result.pulseWidth1 = pulseWidth1;
    result.pulseWidth2 = pulseWidth2;
    if ( verboseLevel > 5 )
        qDebug() << "     nextSlope:"
                 << "/\\"[ int( nextSlope ) ] << "triggeredPositionRaw:" << triggeredPositionRaw;
    return result.triggeredPosition;
}


bool HantekDsoControl::provideTriggeredData() {
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::provideTriggeredData()" << result.tag;
    static DSOsamples triggeredResult; // storage for last triggered trace samples
    if ( result.triggeredPosition ) {  // live trace has triggered
        // Use this trace and save it also
        triggeredResult.data = result.data;
        triggeredResult.samplerate = result.samplerate;
        triggeredResult.clipped = result.clipped;
        triggeredResult.triggeredPosition = result.triggeredPosition;
        result.liveTrigger = true;
    } else if ( controlsettings.trigger.mode == Dso::TriggerMode::NORMAL ) { // Not triggered in NORMAL mode
        // Use saved trace (even if it is empty)
        result.data = triggeredResult.data;
        result.samplerate = triggeredResult.samplerate;
        result.clipped = triggeredResult.clipped;
        result.triggeredPosition = triggeredResult.triggeredPosition;
        result.liveTrigger = false; // show red "TR" top left
    } else {                        // Not triggered and not NORMAL mode
        // Use the free running trace, discard history
        triggeredResult.data.clear();          // discard trace
        triggeredResult.triggeredPosition = 0; // not triggered
        result.liveTrigger = false;            // show red "TR" top left
    }
    return result.liveTrigger;
}


/// \brief Updates the interval of the periodic thread timer.
void HantekDsoControl::updateInterval() {
    // Check the current oscilloscope state everytime 25% of the time
    //  the buffer should be refilled (-> acquireInterval in ms)
    // Use real 100% rate for demo device
    int sampleInterval = int( getSamplesize() * 1000.0 / controlsettings.samplerate.current );
    // Slower update reduces CPU load but it worsens the triggering of rare events
    // Display can be filled at slower rate (not faster than displayInterval)
    if ( scope ) // init is done
        acquireInterval = int( 1000 * scope->horizontal.acquireInterval );
    else
        acquireInterval = 1;
#ifdef Q_PROCESSOR_ARM
    displayInterval = 200; // update display at least every 200 ms
#else
    displayInterval = 100; // update display at least every 100 ms
#endif
    acquireInterval = qMin( qMax( sampleInterval, acquireInterval ), 100 ); // at least every 100 ms
}


/// \brief State machine for the device communication
void HantekDsoControl::stateMachine() {
    static int delayDisplay = 0;       // timer for display
    static bool lastTriggered = false; // state of last frame
    static bool skipEven = true;       // even or odd frames were skipped
    static unsigned lastTag = 0;

    bool triggered = false;
    if ( verboseLevel > 4 )
        qDebug() << "    HDC::stateMachine()" << raw.tag;

    // we have a sample available ...
    // ... that is either a new sample or we are in free run mode or a new trigger search is needed
    if ( samplingStarted && raw.valid && ( raw.tag != lastTag || raw.freeRun || triggerChanged() ) ) {
        lastTag = raw.tag;
        convertRawDataToSamples(); // process samples, apply gain settings etc.
        QWriteLocker resultLocker( &result.lock );
        if ( !result.freeRunning ) {            // trigger mode != NONE
            searchTriggeredPosition();          // detect trigger point
            triggered = provideTriggeredData(); // present either free running or last triggered trace
        } else { // free running display (uses half sample size -> double display speed for slow sample rates)
            triggered = false;
            result.triggeredPosition = 0;
        }
    } else { // start with correct calibration frequency
        static bool firstTime = true;
        if ( firstTime && scope ) {
            setCalFreq( scope->horizontal.calfreq );
            firstTime = false;
        }
    }
    delayDisplay += qMax( acquireInterval, 1 );
    // always run the display (slowly at t=displayInterval) to allow user interaction
    // ... but update immediately if new triggered data is available after untriggered
    // skip an even number of frames when slope == Dso::Slope::Both
    if ( ( triggered && !lastTriggered )                                 // show new data immediately
         || ( ( delayDisplay >= displayInterval )                        // or wait some time ...
              && ( ( controlsettings.trigger.slope != Dso::Slope::Both ) // ... for ↗ or ↘ slope
                   || skipEven ) ) ) {                                   // and drop even no. of frames
        skipEven = true;                                                 // zero frames -> even
        delayDisplay = 0;
        timestampDebug( QString( "samplesAvailable %1" ).arg( result.tag ) );
        emit samplesAvailable( &result ); // via signal/slot -> PostProcessing::input()
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

    if ( isSampling() ) {
        // Sampling hasn't started, update the expected sample count
        expectedSampleCount = getSampleCount();
        timestampDebug( "Starting to capture" );
        samplingStarted = true;
    }
    updateInterval(); // calculate new acquire timing
    if ( stateMachineRunning ) {
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 4, 0 ) )
        QTimer::singleShot( acquireInterval, this, &HantekDsoControl::stateMachine );
#else
        QTimer::singleShot( acquireInterval, this, SLOT( stateMachine() ) );
#endif
    }
}


void HantekDsoControl::addCommand( ControlCommand *newCommand, bool pending ) {
    newCommand->pending = pending;
    control[ newCommand->code ] = newCommand;
    newCommand->next = firstControlCommand;
    firstControlCommand = newCommand;
}


// sending control commands to the scope:
// format: "cc <CC> <DD> <DD> ..."
// <CC> = control code, e.g. E6 (SETCALFREQ)
// <DD> = data, e.g. 01 = 1kHz or 69 (= 105 dec) = 50 Hz
// all <CC> and <DD> uint8_t values must consist of 2 hex encoded digits
// come here with validated strings from mainwindow.cpp
Dso::ErrorCode HantekDsoControl::stringCommand( const QString &commandString ) {
    if ( deviceNotConnected() )
        return Dso::ErrorCode::CONNECTION;
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0 ) )
    QStringList commandParts = commandString.split( ' ', Qt::SkipEmptyParts );
#else
    QStringList commandParts = commandString.split( ' ', QString::SkipEmptyParts );
#endif
    if ( commandParts.count() < 1 )
        return Dso::ErrorCode::PARAMETER;
    if ( commandParts[ 0 ] == "cc" || commandParts[ 0 ] == "CC" ) {
        if ( commandParts.count() < 2 )
            return Dso::ErrorCode::PARAMETER;

        uint8_t codeIndex = 0;
        hexParse( commandParts[ 1 ], &codeIndex, 1 );
        QString data = commandString.section( ' ', 2, -1, QString::SectionSkipEmpty );

        if ( !control[ codeIndex ] )
            return Dso::ErrorCode::UNSUPPORTED;

        QString name = "";
        if ( codeIndex >= 0xe0 && codeIndex <= 0xe6 )
            name = controlNames[ codeIndex - 0xe0 ];

        ControlCommand *c = modifyCommand< ControlCommand >( ControlCode( codeIndex ) );
        hexParse( data, c->data(), unsigned( c->size() ) );
        if ( verboseLevel > 2 )
            qDebug().noquote() << "  " + commandParts[ 0 ]
                               << QString( "0x%1 (%2) %3" )
                                      .arg( QString::number( codeIndex, 16 ), name, decDump( c->data(), unsigned( c->size() ) ) );
        if ( int( c->size() ) != commandParts.count() - 2 )
            return Dso::ErrorCode::PARAMETER;
        return Dso::ErrorCode::NONE;
    } else if ( commandParts[ 0 ] == "freq" ) {     // simple example for manual frequency command "freq nn"
        if ( commandParts.count() < 2 )             // command and one parameter needed
            return Dso::ErrorCode::PARAMETER;       // .. otherwise -> error
        unsigned freq = commandParts[ 1 ].toUInt(); // decode parameter as one decimal value into freq
        if ( !freq || freq > 100000 )               // parameter valid?
            return Dso::ErrorCode::PARAMETER;       // .. otherwise -> error
        if ( verboseLevel > 2 )                     // verbose enough?
            qDebug( "  freq %d", freq );            // .. show the parameter
        return setCalFreq( freq );                  // and call the scope function
    }
    return Dso::ErrorCode::UNSUPPORTED;
}
