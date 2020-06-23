// SPDX-License-Identifier: GPL-2.0+

#include "modelDDS120.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "usb/scopedevice.h"
#include <QDir>
#include <QSettings>

#define VERBOSE 0

using namespace Hantek;

static ModelDDS120 modelInstance_120;

static void initSpecifications( Dso::ControlSpecification &specification ) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in hantekdsocontrol.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()

    // HW gain, voltage steps in V/screenheight (ranges 20,50,100,200,500,1000,2000,5000 mV)
    // DDS120 has gainsteps 20x, 10x, 5x, 2x and 1x (as well as also 4x)
    // Hantek has only 10x, 5x, 2x, 1x
    specification.gain = {
        //              ID, HW gain, voltage/div
        {20, 20e-3},  // 0     20       20mV/div
        {20, 50e-3},  // 1     20       50mV/div
        {10, 100e-3}, // 2     10      100mV/div
        {5, 200e-3},  // 3      5      200mV/div
        {2, 500e-3},  // 4      2      500mV/div
        {1, 1.00},    // 5      1         1V/div
        {1, 2.00},    // 6      1         2V/div
        {1, 5.00}     // 7      1         5V/div
    };

    // This data was based on testing and depends on divider.
    // The sample value at the top of the screen with gain error correction
    // TODO: check if 20x is possible for 1st and 2nd value
    // double the values accordingly 32 -> 64 & 80 -> 160 and change 10 -> 20 in specification.gain below
    // DDS120 correction factors x20: 0.80, x10: 0.80, x5: 0.77, x2: 0.86, x1: 0.83
    // theoretical voltageScales x20: 500,  x10: 250,  x5: 125,  x2: 50,   x1: 25
    specification.voltageScale[ 0 ] = {400, 400, 200, 96, 43, 21, 21, 21};
    specification.voltageScale[ 1 ] = {400, 400, 200, 96, 43, 21, 21, 21};
    // specification.voltageScale[ 0 ] = {64, 160, 160, 155, 170, 165, 330, 820};
    // specification.voltageScale[ 1 ] = {64, 160, 160, 155, 170, 165, 330, 820};
    // theoretical offset, will be corrected by individual config file
    specification.voltageOffset[ 0 ] = {0, 0, 0, 0, 0, 0, 0, 0};
    specification.voltageOffset[ 1 ] = {0, 0, 0, 0, 0, 0, 0, 0};

    // read the real calibration values from file
    const char *ranges[] = {"20mV", "50mV", "100mV", "200mV", "500mV", "1000mV", "2000mV", "5000mV"};
    const char *channels[] = {"ch0", "ch1"};
    // printf( "read config file\n" );
    const unsigned RANGES = 8;

    // rename the old *.conf to *.ini to use the ini file search also for Windows
    QString calFileName = QDir::homePath() + "/.config/OpenHantek/modelDSO6022";
    QFile calFile( calFileName + ".conf" );
    if ( calFile.exists() ) {
        calFileName += ".ini";
        calFile.rename( calFileName );
    }
    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings settings( "OpenHantek", "modelDDS120" );
    // Linux, Unix, macOS: "$HOME/.config/OpenHantek/modelDDS120.ini"
    // Windows: "%APPDATA%\OpenHantek\modelDDS120.ini"

    settings.beginGroup( "gain" );
    for ( unsigned ch = 0; ch < 2; ch++ ) {
        settings.beginGroup( channels[ ch ] );
        for ( unsigned iii = 0; iii < RANGES; iii++ ) {
            double calibration = settings.value( ranges[ iii ], 0.0 ).toDouble();
            if ( bool( calibration ) )
                specification.voltageScale[ ch ][ iii ] /= calibration;
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // gain

    settings.beginGroup( "offset" );
    for ( unsigned ch = 0; ch < 2; ch++ ) {
        settings.beginGroup( channels[ ch ] );
        for ( unsigned iii = 0; iii < RANGES; iii++ ) {
            // settings.setValue( ranges[ iii ], iii );
            // set to 0x80 if no value from conf file
            int offset = settings.value( ranges[ iii ], "255" ).toInt();
            if ( offset != 255 ) // value exists in config file
                specification.voltageOffset[ ch ][ iii ] = 0x80 - offset;
            // printf( "%d-%d: %d %d\n", ch, iii, offset, specification.voltageOffset[ ch ][ iii ] );
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // offset
    QSettings::setDefaultFormat( QSettings::NativeFormat );


    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.recordLengths = {UINT_MAX};
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.recordLengths = {UINT_MAX};

    // This model uses the sigrok firmware that has a slightly different coding for the sample rate than my Hantek6022API version.
    // 10=100k, 20=200k, 50=500k, 11=10M (Hantek: 110=100k, 120=200k, 150=500k, 10=10M)

    // 48M is unstable in 1 channel mode
    // 24M, 30M and 48M are unstable in 2 channel mode

    specification.fixedSampleRates = {
        // samplerate, sampleId, downsampling
        {10e3, 1, 100}, // 100x downsampling from  1 MS/s!
        {20e3, 2, 100}, // 100x downsampling from  2 MS/s!
        {50e3, 5, 100}, // 100x downsampling from  5 MS/s!
        {100e3, 8, 80}, //  80x downsampling from  8 MS/s
        {200e3, 8, 40}, //  40x downsampling from  8 MS/s
        {500e3, 8, 16}, //  16x downsampling from  8 MS/s
        {1e6, 8, 8},    //   8x downsampling from  8 MS/s
        {2e6, 8, 4},    //   4x downsampling from  8 MS/s
        {5e6, 15, 3},   //   3x downsampling from 15 MS/s
        {10e6, 11, 1},  // no downsampling, 11 means 10 MS/s
        {15e6, 15, 1},  // no downsampling
        {24e6, 24, 1},  // no downsampling
        {30e6, 30, 1},  // no downsampling
        {48e6, 48, 1}   // no downsampling
    };


    specification.couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    specification.hasACcoupling = true; // DDS120 has AC coupling
    specification.triggerModes = {
        Dso::TriggerMode::NONE,
        Dso::TriggerMode::AUTO,
        Dso::TriggerMode::NORMAL,
        Dso::TriggerMode::SINGLE,
    };
    specification.fixedUSBinLength = 0;
    // use calibration frequency steps of modified sigrok FW (<= 20 kHz)
    specification.calfreqSteps = {50, 60, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    specification.hasCalibrationEEPROM = false;
}

static void applyRequirements_( HantekDsoControl *dsoControl ) {
    dsoControl->addCommand( new ControlSetGain_CH1() );    // 0xE0
    dsoControl->addCommand( new ControlSetGain_CH2() );    // 0xE1
    dsoControl->addCommand( new ControlSetSamplerate() );  // 0xE2
    dsoControl->addCommand( new ControlStartSampling() );  // 0xE3
    dsoControl->addCommand( new ControlSetNumChannels() ); // 0xE4
    dsoControl->addCommand( new ControlSetCoupling() );    // 0xE5
    dsoControl->addCommand( new ControlSetCalFreq() );     // 0xE6
}

//                                        VID/PID active  VID/PID no FW   FW ver  FW name  Scope name
//                                        |------------|  |------------|  |----|  |------|  |------|
ModelDDS120::ModelDDS120()
    : DSOModel( ID, 0x04b5, 0x0120, 0x8102, 0x8102, 0x0100, "dds120", "DDS120", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelDDS120::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }
