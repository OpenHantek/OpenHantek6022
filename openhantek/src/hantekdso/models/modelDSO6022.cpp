// SPDX-License-Identifier: GPL-2.0+

#include "modelDSO6022.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "usb/scopedevice.h"
#include <QDir>
#include <QSettings>

#define VERBOSE 0

using namespace Hantek;

static ModelDSO6022BE modelInstance_6022be;
static ModelDSO6022BL modelInstance_6022bl;

static ModelDSO2020 modelInstance_2020;

static void initSpecifications( Dso::ControlSpecification &specification ) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in hantekdsocontrol.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()

    // HW gain, voltage steps in V/div (ranges 20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = {{10, 20e-3}, {10, 50e-3}, {10, 100e-3}, {5, 200e-3}, {2, 500e-3}, {1, 1.00}, {1, 2.00}, {1, 5.00}};

    // Define the scaling between ADC sample values and real input voltage
    // Everything is scaled on the full screen height (8 divs)
    // The voltage/div setting:      20m   50m  100m  200m  500m    1V    2V    5V
    // Equivalent input voltage:   0.16V  0.4V  0.8V  1.6V    4V    8V   16V   40V
    // Theoretical gain setting:     x10   x10   x10   x5    x2     x1    x1    x1
    // mV / digit:                     4     4     4     8    20    40    40    40
    // The real input front end introduces a gain error
    // Input divider: 100/1009 = 1% too low display
    // Amplifier gain: x1 (ok), x2 (ok), x5.1 (2% too high), x10.1 (1% too high)
    // Overall resulting gain: x1 1% too low, x2 1% to low, x5 1% to high, x10 ok
    // The sample value for full screen (8 divs) with theoretical gain setting
    // specification.voltageScale[ 0 ] = {40, 100, 200, 202, 198, 198, 396, 990};
    // specification.voltageScale[ 1 ] = {40, 100, 200, 202, 198, 198, 396, 990};
    specification.voltageScale[ 0 ] = {250, 250, 250, 126.25, 49.50, 24.75, 24.75, 24.75};
    specification.voltageScale[ 1 ] = {250, 250, 250, 126.25, 49.50, 24.75, 24.75, 24.75};
    specification.voltageOffset[ 0 ] = {0, 0, 0, 0, 0, 0, 0, 0};
    specification.voltageOffset[ 1 ] = {0, 0, 0, 0, 0, 0, 0, 0};
    // Gain and offset can be corrected by individual config values from EEPROM or file

    // read the real calibration values from file
    const char *ranges[] = {"20mV", "50mV", "100mV", "200mV", "500mV", "1000mV", "2000mV", "5000mV"};
    const char *channels[] = {"ch0", "ch1"};
    // printf( "read config file\n" );
    const unsigned RANGES = 8;

    // rename the previous *.conf to *.ini to use the ini file search also for Windows
    QString calFileName = QDir::homePath() + "/.config/OpenHantek/modelDSO6022";
    QFile calFile( calFileName + ".conf" );
    if ( calFile.exists() ) {
        calFileName += ".ini";
        calFile.rename( calFileName );
    }
    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings settings( "OpenHantek", "modelDSO6022" );
    // Linux, Unix, macOS: "$HOME/.config/OpenHantek/modelDSO6022.ini"
    // Windows: "%APPDATA%\OpenHantek\modelDSO6022.ini"

    settings.beginGroup( "gain" );
    for ( unsigned ch = 0; ch < 2; ch++ ) {
        settings.beginGroup( channels[ ch ] );
        for ( unsigned iii = 0; iii < RANGES; iii++ ) {
            double gain = settings.value( ranges[ iii ], "0.0" ).toDouble();
            // printf( "ch%d %s: gain = %g\n", ch, ranges[ iii ], gain );
            if ( bool( gain ) )
                specification.voltageScale[ ch ][ iii ] /= gain;
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // gain

    settings.beginGroup( "offset" );
    for ( unsigned ch = 0; ch < 2; ch++ ) {
        settings.beginGroup( channels[ ch ] );
        for ( unsigned iii = 0; iii < RANGES; iii++ ) {
            // set to 0x00 if no value from conf file
            int offset = settings.value( ranges[ iii ], "255" ).toInt();
            // printf( "ch%d %s: offset = %d\n", ch, ranges[ iii ], offset );
            if ( offset != 255 ) // value exists in config file
                specification.voltageOffset[ ch ][ iii ] = 0x80 - offset;
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // offset
    QSettings::setDefaultFormat( QSettings::NativeFormat );

    // Possible raw sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 20k, 50k, 64k, 100k, 200k, 500k, 1M, 2M, 3M, 4M, 5M, 6M, 8M, 10M, 12M, 15M, 16M, 24M, 30M (, 48M)
    // 48M is unusable in 1 channel mode due to massive USB overrun
    // 24M, 30M and 48M are unusable in 2 channel mode
    // these unstable settings are disabled
    // Lower effective sample rates < 10 MS/s use oversampling to increase the SNR

    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.recordLengths = {UINT_MAX};
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.recordLengths = {UINT_MAX};

    specification.fixedSampleRates = {
        // samplerate, sampleId, downsampling
        {200, 102, 100},  // very slow! 100x downsampling from 20, 50 kS/s
        {500, 105, 100},  // very slow!
        {1e3, 110, 100},  // slow! 100x downsampling from 100, 200, 500 kS/s
        {2e3, 120, 100},  // slow!
        {5e3, 150, 100},  // slow!
        {10e3, 1, 100},   // 100x downsampling from 1, 2, 5, 10 MS/s
        {20e3, 2, 100},   //
        {50e3, 5, 100},   //
        {100e3, 10, 100}, //
        {200e3, 10, 50},  // 50x, 20x 10x, 5x, 2x downsampling from 10 MS/s
        {500e3, 10, 20},  //
        {1e6, 10, 10},    //
        {2e6, 10, 5},     //
        {5e6, 10, 2},     //
        {10e6, 10, 1},    // no oversampling
        {12e6, 12, 1},    //
        {15e6, 15, 1},    //
        {24e6, 24, 1},    //
        {30e6, 30, 1},    //
        {48e6, 48, 1}     //
    };

#ifdef HANTEK_AC
    // requires AC/DC HW mod like DDS120, enable with "cmake -D HANTEK_AC=1 .."
    specification.couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
#else
    specification.couplings = {Dso::Coupling::DC};
#endif
    specification.triggerModes = {
        Dso::TriggerMode::NONE,
        Dso::TriggerMode::AUTO,
        Dso::TriggerMode::NORMAL,
        Dso::TriggerMode::SINGLE,
    };
    specification.fixedUSBinLength = 512;

    // calibration frequency (requires >FW0206)
    specification.calfreqSteps = {50, 60, 100, 200, 500, 1e3, 2e3, 5e3, 10e3, 20e3, 50e3, 100e3};
    specification.hasCalibrationEEPROM = true;
}

static void applyRequirements_( HantekDsoControl *dsoControl ) {
    dsoControl->addCommand( new ControlSetGain_CH1() );    // 0xE0
    dsoControl->addCommand( new ControlSetGain_CH2() );    // 0xE1
    dsoControl->addCommand( new ControlSetSamplerate() );  // 0xE2
    dsoControl->addCommand( new ControlStartSampling() );  // 0xE3
    dsoControl->addCommand( new ControlSetNumChannels() ); // 0xE4
    dsoControl->addCommand( new ControlSetCoupling() );    // 0xE5 (no effect w/o AC/DC HW mod)
    dsoControl->addCommand( new ControlSetCalFreq() );     // 0xE6
}


// Hantek DSO-6022BE (this is the base model)
//
//                                              VID/PID active  VID/PID no FW   FW ver    FW name     Scope name
//                                              |------------|  |------------|  |----|  |---------|  |----------|
ModelDSO6022BE::ModelDSO6022BE()
    : DSOModel( ID, 0x04b5, 0x6022, 0x04b4, 0x6022, 0x0206, "dso6022be", "DSO-6022BE", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelDSO6022BE::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }


// Hantek DSO-6022BL (scope or logic analyzer)
ModelDSO6022BL::ModelDSO6022BL()
    : DSOModel( ID, 0x04b5, 0x602a, 0x04b4, 0x602a, 0x0206, "dso6022bl", "DSO-6022BL", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelDSO6022BL::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }


// Voltcraft DSO-2020 USB Oscilloscope (HW is identical to 6022)
// Scope starts up as model DS-2020 (VID/PID = 04b4/2020) but loads 6022BE firmware and looks like a 6022BE
ModelDSO2020::ModelDSO2020()
    : DSOModel( ID, 0x04b5, 0x6022, 0x04b4, 0x2020, 0x0206, "dso6022be", "DSO-2020", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelDSO2020::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }


// two test cases with simple EZUSB board (LCsoft) without EEPROM or with Saleae VID/PID in EEPROM
// after loading the FW they look like a 6022BE (without useful sample values as Port B and D are left open)
// LCSOFT_TEST_BOARD is #defined/#undefined in modelDSO6022.h

#ifdef LCSOFT_TEST_BOARD

static ModelEzUSB modelInstance_EzUSB;
static ModelSaleae modelInstance_Saleae;


// LCSOFT without EEPROM reports EzUSB VID/PID
ModelEzUSB::ModelEzUSB()
    : DSOModel( ID, 0x04b5, 0x6022, 0x04b4, 0x8613, 0x0206, "dso6022be", "LCsoft-EzUSB", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
    specification.hasCalibrationEEPROM = false; // (big) EEPROM, disabled by address jumper
}

void ModelEzUSB::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }


// Saleae VID/PID in EEPROM
ModelSaleae::ModelSaleae()
    : DSOModel( ID, 0x04b5, 0x6022, 0x0925, 0x3881, 0x0206, "dso6022be", "LCsoft-Saleae", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
    specification.hasCalibrationEEPROM = false; // we have a big EEPROM
}


void ModelSaleae::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }

#endif
