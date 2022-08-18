// SPDX-License-Identifier: GPL-2.0-or-later

#include "modelDEMO.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "usb/scopedevice.h"
#include <QDebug>
#include <QDir>
#include <QSettings>

#define VERBOSE 0

using namespace Hantek;

static ModelDEMO modelInstance_DEMO;

static void initSpecifications( Dso::ControlSpecification &specification ) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in hantekdsocontrol.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()

    // HW gain, voltage steps in V/div (ranges 20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = { { 10, 20e-3 }, { 10, 50e-3 }, { 10, 100e-3 }, { 5, 200e-3 },
                           { 2, 500e-3 }, { 1, 1.00 },   { 1, 2.00 },    { 1, 5.00 } };

    // Define the scaling between ADC sample values and real input voltage
    // Everything is scaled on the full screen height (8 divs)
    // The voltage/div setting:      20m   50m  100m  200m  500m    1V    2V    5V
    // Equivalent input voltage:   0.16V  0.4V  0.8V  1.6V    4V    8V   16V   40V
    // Theoretical gain setting:     x10   x10   x10   x5    x2     x1    x1    x1
    // mV / digit:                     4     4     4     8    20    40    40    40
    // The sample value for full screen (8 divs) with theoretical gain setting
    specification.voltageScale[ 0 ] = { 250, 250, 250, 125, 50, 25, 25, 25 };
    specification.voltageScale[ 1 ] = { 250, 250, 250, 125, 50, 25, 25, 25 };

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
        { 100, 102, 200 },  // very slow! 200x downsampling from 20 kS/s
        { 200, 104, 200 },  // very slow! 200x downsampling from 40 kS/s
        { 500, 110, 200 },  // very slow! 200x downsampling from 100 kS/s
        { 1e3, 120, 200 },  // slow! 200x downsampling from 200 kS/s
        { 2e3, 140, 200 },  // slow! 200x downsampling from 400 kS/s
        { 5e3, 1, 200 },    // slow! 200x downsampling from 1 MS/s
        { 10e3, 1, 100 },   // 100x downsampling from 1, 2, 5, 10 MS/s
        { 20e3, 2, 100 },   //
        { 50e3, 5, 100 },   //
        { 100e3, 10, 100 }, //
        { 200e3, 10, 50 },  // 50x, 20x 10x, 5x, 2x downsampling from 10 MS/s
        { 500e3, 10, 20 },  //
        { 1e6, 10, 10 },    //
        { 2e6, 10, 5 },     //
        { 5e6, 10, 2 },     //
        { 10e6, 10, 1 },    // no oversampling
        { 12e6, 12, 1 },    //
        { 15e6, 15, 1 },    //
        { 24e6, 24, 1 },    //
        { 30e6, 30, 1 },    //
        { 48e6, 48, 1 }     //
    };

    // AC requires AC/DC HW mod like DDS120, enable with "cmake -D HANTEK_AC=1 .." or config option
    specification.couplings = { Dso::Coupling::DC, Dso::Coupling::AC };
    specification.triggerModes = {
        Dso::TriggerMode::AUTO,
        Dso::TriggerMode::NORMAL,
        Dso::TriggerMode::SINGLE,
        Dso::TriggerMode::ROLL,
    };
    specification.fixedUSBinLength = 0;

    // calibration frequency output is non functional, just show the spinbox
    specification.calfreqSteps = { 32,  40,   50,   60,   80,    100,   120,  160,   200,  250,  300,  400,
                                   500, 600,  800,  1e3,  1.2e3, 1.6e3, 2e3,  2.5e3, 3e3,  4e3,  5e3,  6e3,
                                   8e3, 10e3, 12e3, 16e3, 20e3,  25e3,  30e3, 40e3,  50e3, 60e3, 80e3, 100e3 };
    specification.hasCalibrationEEPROM = true;
    specification.isDemoDevice = true;
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


// DEMO similar to Hantek DSO-6022BE
//
//                                     VID/PID active  VID/PID no FW   FW ver  FW name Scope name
//                                     |------------|  |------------|  |----|  |----|  |----|
ModelDEMO::ModelDEMO() : DSOModel( ID, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, "demo", "DEMO", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelDEMO::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }
