// SPDX-License-Identifier: GPL-2.0-or-later

#include "modelMDSO.h"
#include "hantekdsocontrol.h"
#include "hantekprotocol/controlStructs.h"
#include "usb/scopedevice.h"
#include <QDebug>
#include <QDir>
#include <QSettings>

#define VERBOSE 0

using namespace Hantek;

static ModelMDSO modelInstance_1D50;

static void initSpecifications( Dso::ControlSpecification &specification ) {

    // HW gain, voltage steps in V/div (ranges 20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = { { 10, 20e-3 }, { 10, 50e-3 }, { 10, 100e-3 }, { 5, 200e-3 },
                           { 2, 500e-3 }, { 1, 1.00 },   { 1, 2.00 },    { 1, 5.00 } };


    // Define the scaling between ADC sample values and real input voltage
    // Everything is scaled on the full screen height (8 divs)
    // The voltage/div setting:         20m  50m 100m 200m   500m     1V     2V     5V
    // Equivalent input voltage:      0.16V 0.4V 0.8V 1.6V     4V     8V    16V    40V
    // Theoretical gain setting:        x10  x10  x10   x5     x2     x1     x1     x1
    // mV / digit:                        4    4    4    8     20     40     40     40
    specification.voltageScale[ 0 ] = { 250, 250, 250, 126.25, 49.50, 24.75, 24.75, 24.75 };
    specification.voltageScale[ 1 ] = { 250, 250, 250, 126.25, 49.50, 24.75, 24.75, 24.75 };
    // Gain and offset can be corrected by individual config values from file (device has no calibration EEPROM)


    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.recordLengths = { UINT_MAX };

    // This model uses the sigrok firmware that has a slightly different coding for the sample rate than my Hantek6022API version.
    // 10=100k, 20=200k, 50=500k, 11=10M (Hantek: 110=100k, 120=200k, 150=500k, 10=10M)

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


    specification.couplings = { Dso::Coupling::DC, Dso::Coupling::AC };
    specification.hasACcoupling = false; // MDSO has AC coupling
    specification.triggerModes = {
        Dso::TriggerMode::AUTO,
        Dso::TriggerMode::NORMAL,
        Dso::TriggerMode::SINGLE,
        Dso::TriggerMode::ROLL,
    };
    specification.fixedUSBinLength = 0;
    // use calibration frequency steps of modified sigrok FW (<= 20 kHz)
    specification.calfreqSteps = { 50, 60, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
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

//                                     VID/PID active  VID/PID no FW   FW ver  FW name Scope name
//                                     |------------|  |------------|  |----|  |----|  |----|
ModelMDSO::ModelMDSO() : DSOModel( ID, 0x1d50, 0x608e, 0xd4a2, 0x5660, 0x0001, "mdso", "MDSO", Dso::ControlSpecification( 2 ) ) {
    initSpecifications( specification );
}

void ModelMDSO::applyRequirements( HantekDsoControl *dsoControl ) const { applyRequirements_( dsoControl ); }
