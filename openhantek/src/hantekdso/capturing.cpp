// SPDX-License-Identifier: GPL-2.0+

// #define TIMESTAMPDEBUG

#include "capturing.h"
#include "usb/scopedevice.h"
#include <QDebug>
#include <cmath>


static std::vector< QString > controlNames = {"SETGAIN_CH1",    "SETGAIN_CH2", "SETSAMPLERATE", "STARTSAMPLING",
                                              "SETNUMCHANNELS", "SETCOUPLING", "SETCALFREQ"};

Capturing::Capturing( HantekDsoControl *hdc ) : hdc( hdc ) { hdc->capturing = true; }


void Capturing::run() {
    forever {
        if ( !hdc->capturing || QThread::currentThread()->isInterruptionRequested() ) {
            hdc->quitSampling();     // stop the scope
            hdc->stopStateMachine(); // stop the state machine
            return;                  // stop this thread
        }
        if ( hdc->scope ) { // device is initialized
            capture();
            // add user defined hold-off time to lower CPU load
            QThread::msleep( unsigned( 1000 * hdc->scope->horizontal.acquireInterval ) );
        }
    }
}


double id2sr( uint8_t timediv ) {
    if ( timediv < 100 )
        return timediv * 1e6;
    return ( timediv - 100 ) * 1e4;
}


void Capturing::xferSamples() {
    QWriteLocker locker( &hdc->raw.lock );
    if ( !rollMode )
        swap( data, hdc->raw.data );
    hdc->raw.channels = channels;
    hdc->raw.samplerate = samplerate;
    hdc->raw.oversampling = oversampling;
    hdc->raw.gainValue[ 0 ] = gainValue[ 0 ];
    hdc->raw.gainValue[ 1 ] = gainValue[ 1 ];
    hdc->raw.gainIndex[ 0 ] = gainIndex[ 0 ];
    hdc->raw.gainIndex[ 1 ] = gainIndex[ 1 ];
    hdc->raw.rollMode = rollMode;
    hdc->raw.valid = valid;
    hdc->raw.tag = tag;
}


void Capturing::capture() {
    if ( !hdc->samplingStarted )
        return;
    int errorCode;
    // Send all pending control commands
    ControlCommand *controlCommand = hdc->firstControlCommand;
    while ( controlCommand ) {
        if ( controlCommand->pending ) {
            switch ( int( controlCommand->code ) ) {
            case uint8_t( ControlCode::CONTROL_SETGAIN_CH1 ):
                gainValue[ 0 ] = controlCommand->data()[ 0 ];
                gainIndex[ 0 ] = controlCommand->data()[ 1 ];
                break;
            case uint8_t( ControlCode::CONTROL_SETGAIN_CH2 ):
                gainValue[ 1 ] = controlCommand->data()[ 0 ];
                gainIndex[ 1 ] = controlCommand->data()[ 1 ];
                break;
            case uint8_t( ControlCode::CONTROL_SETNUMCHANNELS ):
                if ( realSlow ) // force 2 channels for slow samplings where roll mode is possible
                    *controlCommand->data() = 2;
                channels = *controlCommand->data();
                break;
            case uint8_t( ControlCode::CONTROL_SETSAMPLERATE ):
                samplerate = id2sr( controlCommand->data()[ 0 ] );
                uint8_t sampleIndex = controlCommand->data()[ 1 ];
                oversampling = uint8_t( hdc->specification->fixedSampleRates[ sampleIndex ].oversampling );
                effectiveSamplerate = hdc->specification->fixedSampleRates[ sampleIndex ].samplerate;
                realSlow = effectiveSamplerate < 10e3;
                if ( realSlow ) // always switch to two channels if roll mode is possible
                    hdc->modifyCommand< ControlSetNumChannels >( ControlCode::CONTROL_SETNUMCHANNELS )->setNumChannels( 2 );
                break;
            }
            QString name = "";
            if ( controlCommand->code >= 0xe0 && controlCommand->code <= 0xe6 )
                name = controlNames[ controlCommand->code - 0xe0 ];
            timestampDebug( QString( "Sending control command 0x%1 (%2):%3" )
                                .arg( QString::number( controlCommand->code, 16 ), name,
                                      hexdecDump( controlCommand->data(), unsigned( controlCommand->size() ) ) ) );
            if ( hdc->scopeDevice->isRealHW() ) { // do the USB communication with scope HW
                errorCode = hdc->scopeDevice->controlWrite( controlCommand );
                if ( errorCode < 0 ) {
                    qWarning( "Sending control command %2x failed: %s", uint8_t( controlCommand->code ),
                              libUsbErrorString( errorCode ).toLocal8Bit().data() );

                    if ( errorCode == LIBUSB_ERROR_NO_DEVICE ) {
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
    valid = true;
    rollMode = hdc->triggerModeNONE() && realSlow;
    // sample step by step into the target if rollMode, else buffer and switch one big block
    dp = rollMode ? &hdc->raw.data : &data;
    rawSamplesize = hdc->grossSampleCount( hdc->getSamplesize() * oversampling ) * channels;
    dp->resize( rawSamplesize, 0x80 );
    if ( tag && rollMode ) // in roll mode transfer settings immediately
        xferSamples();
    ++tag;
    if ( hdc->scopeDevice->isRealHW() ) {
        received = getRealSamples();
    } else {
        received = getDemoSamples();
    }
    if ( received != rawSamplesize ) {
        // qDebug() << "retval != rawSamplesize" << received << rawSamplesize;
        auto end = dp->end();
        for ( auto it = dp->begin(); it != end; ++it )
            *it = 0x80; // fill with "zeros"
        valid = false;
    }
    if ( !rollMode ) // in normal capturing mode transfer after capturing one block
        xferSamples();
}


unsigned Capturing::getRealSamples() {
    int errorCode;
    errorCode = hdc->scopeDevice->controlWrite( hdc->getCommand( ControlCode::CONTROL_STARTSAMPLING ) );
    if ( errorCode < 0 ) {
        qWarning() << "controlWrite: Getting sample data failed: " << libUsbErrorString( errorCode );
        dp->clear();
        return 0;
    }
    // Save raw data to temporary buffer
    timestampDebug( QString( "Request packet %1: %2" ).arg( tag ).arg( rawSamplesize ) );
    int retval = hdc->scopeDevice->bulkReadMulti( dp->data(), rawSamplesize, !realSlow );
    if ( retval < 0 ) {
        qWarning() << "bulkReadMulti: Getting sample data failed: " << libUsbErrorString( retval );
        dp->clear();
        return 0;
    }
    timestampDebug( QString( "Received packet %1: %2" ).arg( tag ).arg( retval ) );
    return unsigned( retval );
}


unsigned Capturing::getDemoSamples() {
    const uint8_t V_zero = 128;   // ADC = 0V
    const uint8_t V_plus_1 = 153; // ADC = 1V
    const uint8_t V_plus_2 = 178; // ADC = 2V
    const uint8_t V_minus_2 = 78; // ADC = -2V
    static uint8_t ch1 = V_zero;
    static uint8_t ch2 = V_zero;
    static int counter = 0;
    unsigned received = 0;
    timestampDebug( QString( "Request dummy packet %1: %2" ).arg( tag ).arg( rawSamplesize ) );
    dp->resize( rawSamplesize, 0x80 );
    auto end = dp->end();
    int deltaT = 99;
    // deltaT (=99) defines the frequency of the dummy signals:
    // ch1 = 1 kHz and ch2 = 500 Hz
    // uncomment the next two lines to disable sample slowdown for low sample rates
    // if ( samplerate < 10e6 )
    //    deltaT = int( round( deltaT * samplerate / 10e6 ) );
    // adapt demo samples for high sample rates >10 MS/s
    if ( samplerate > 10e6 )
        deltaT = int( round( deltaT * samplerate / 10e6 ) );
    int block = 0;
    for ( auto it = dp->begin(); it != end; ++it ) {
        if ( ++counter >= deltaT ) {
            counter = 0;
            if ( --ch1 < V_minus_2 ) {
                ch1 = V_plus_2;
                ch2 = ch2 <= V_plus_1 ? V_plus_2 : V_zero;
            }
        }
        *it = ch1;
        ++received;
        if ( 2 == channels ) {
            *++it = ch2;
            ++received;
        }
        if ( ++block >= 1024 ) {
            block = 0;
            QThread::usleep( unsigned( 1024 * 1e6 / samplerate ) );
            if ( !hdc->capturing || hdc->scopeDevice->hasStopped() )
                break;
        }
    }
    // qDebug() << unsigned( rawSamplesize * 1000.0 / samplerate / channels );
    // QThread::msleep( unsigned( rawSamplesize * 1000.0 / samplerate / channels ) );
    timestampDebug( QString( "Received dummy packet %1: %2" ).arg( tag ).arg( rawSamplesize ) );
    return received;
}
