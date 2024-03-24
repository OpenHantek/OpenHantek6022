// SPDX-License-Identifier: GPL-2.0-or-later

#include "modelISDS205b.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "usb/scopedevice.h"
#include <QDebug>
#include <QDir>
#include <QSettings>

#include "../res/firmware/dso602x_fw_version.h"


#define VERBOSE 0

using namespace Hantek;

static ModelISDS205B modelInstance_isds205b;


static void initSpecifications( Dso::ControlSpecification &specification ) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in hantekdsocontrol.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()

    // HW gain, voltage steps in V/div (ranges 20,50,100,200,500,1000,2000,5000 mV)
    
    specification.gain = { { 10, 20e-3 }, { 10, 50e-3 }, { 10, 100e-3 }, { 5, 200e-3 },{ 2, 500e-3 }, { 1, 1.00 },   { 1, 2.00 },    { 1, 5.00 } };
    
    // ID and actual gain:
    // 1: 1.1
    // 2: 2
    // 5: 4.9
    // 10: 16
    // Full scale input == +- 5V according to instrustar.
    // For low voltages a relay can be switched, so a 10x attenuator is bypassed. i.e. input at ADC +- 0.5V always

    // Define the scaling between ADC sample values and real input voltage
    // Everything is scaled on the full screen height (8 divs)
    //                              ||WITH LV SWITCH||
    // The voltage/div setting:      20m   50m  100m  200m  500m    1V    2V    5V
    // Equivalent input voltage:   0.16V  0.4V  0.8V  1.6V    4V    8V   16V   40V
    // Theoretical gain setting:     x16   x16   x16  x4.9    x2  x1.1  x1.1  x1.1
           
    // Theoretical values: 4096.  , 4096.  , 4096. , 125.44, 51.2 , 28.16, 28.16, 28.16
    specification.voltageScale[ 0 ] = { 1330, 1330,1330, 85.36, 37.48, 21.78, 21.78, 21.78 }; //digit/V - Channel 1
    specification.voltageScale[ 1 ] = { 1330, 1330,1330, 85.36, 37.48, 21.78, 21.78, 21.78 }; //digit/V - Channel 2

    // Gain and offset can be corrected by individual config values from EEPROM or file

    // Possible raw sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 20k, 40k, 50k, 64k, 100k, 200k, 400k, 500k, 1M, 2M, 3M, 4M, 5M, 6M, 8M, 10M, 12M, 15M, 16M, 24M, 30M (, 48M)
    // 48M is unusable in 1 channel mode due to massive USB overrun
    // 24M, 30M and 48M are unusable in 2 channel mode
    // these unstable settings are disabled
    // Lower effective sample rates < 10 MS/s use oversampling to increase the SNR

    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.recordLengths = { UINT_MAX };

    specification.fixedSampleRates = {
        // samplerate, sampleId, downsampling
        { 10e3, 1, 100 }, // 100x downsampling from  1 MS/s!
        { 20e3, 2, 100 }, // 100x downsampling from  2 MS/s!
        { 50e3, 5, 100 }, // 100x downsampling from  5 MS/s!
        { 100e3, 8, 80 }, //  80x downsampling from  8 MS/s
        { 200e3, 8, 40 }, //  40x downsampling from  8 MS/s
        { 500e3, 8, 16 }, //  16x downsampling from  8 MS/s
        { 1e6, 8, 8 },    //   8x downsampling from  8 MS/s
        { 2e6, 8, 4 },    //   4x downsampling from  8 MS/s
        { 5e6, 15, 3 },   //   3x downsampling from 15 MS/s
        { 10e6, 11, 1 },  // no downsampling, 11 means 10 MS/s
        { 15e6, 15, 1 },  // no downsampling
        { 24e6, 24, 1 },  // no downsampling
        { 30e6, 30, 1 },  // no downsampling
        { 48e6, 48, 1 }   // no downsampling
    };

    // Instrustar has AC Coupling option
    specification.couplings = { Dso::Coupling::DC, Dso::Coupling::AC };
    specification.hasACcoupling = true;

    specification.triggerModes = {
        Dso::TriggerMode::AUTO,
        Dso::TriggerMode::NORMAL,
        Dso::TriggerMode::SINGLE,
        Dso::TriggerMode::ROLL,
    };    
    specification.fixedUSBinLength = 0;
    //Use calibration steps supported by firmware 205b
    specification.calfreqSteps = { 100,1000,10000,25000 };
    specification.hasCalibrationEEPROM = false;

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


// Instrustar ISDS20A/B
//
//                  VID/PID active  VID/PID no FW   FW ver          FW name    Scope name
//                  |------------|  |------------|  |------------|  |-------|  |--------|
ModelISDS205B::ModelISDS205B()
    : DSOModel( ID, 0x1d50, 0x608e, 0xd4a2, 0x5661,  0x0005, "isds205b", "ISDS-205B", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelISDS205B::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }