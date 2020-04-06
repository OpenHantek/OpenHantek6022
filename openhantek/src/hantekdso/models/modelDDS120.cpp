// SPDX-License-Identifier: GPL-2.0+

#include <QDir>
#include <QSettings>
#include "modelDDS120.h"
#include "usb/usbdevice.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

#define VERBOSE 0

using namespace Hantek;

static ModelDDS120 modelInstance_120;

static void initSpecifications(Dso::ControlSpecification& specification) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in modelDDS120.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()
    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.maxDownsampler = 100;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.maxDownsampler = 100;
    specification.samplerate.multi.recordLengths = { UINT_MAX };
    specification.bufferDividers = { 1000 , 1 , 1 };
    // This data was based on testing and depends on divider.
    // The sample value at the top of the screen with gain error correction
    // TODO: check if 20x is possible for 1st and 2nd value
    // double the values accordingly 32 -> 64 & 80 -> 160 and change 10 -> 20 in specification.gain below
    specification.voltageScale[0] = { 64 , 160 , 160 , 155 , 170 , 165 , 330 , 820 };
    specification.voltageScale[1] = { 64 , 160 , 160 , 155 , 170 , 165 , 330 , 820 };
    // theoretical offset, will be corrected by individual config file
    specification.voltageOffset[0] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    specification.voltageOffset[1] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // read the real calibration values from file
    const char* ranges[] = { "20mV", "50mV","100mV", "200mV", "500mV", "1000mV", "2000mV", "5000mV" }; 
    const char* channels[] = { "ch0", "ch1" };
    //printf( "read config file\n" );
    const unsigned RANGES = 8;
    QSettings settings( QDir::homePath() + "/.config/OpenHantek/modelDDS120.conf", QSettings::IniFormat );

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
            //settings.setValue( ranges[ iii ], iii );
            // set to 0x80 if no value from conf file
            int offset = settings.value( ranges[ iii ], "255" ).toInt();
            if ( offset != 255 ) // value exists in config file
                specification.voltageOffset[ ch ][ iii ] = 0x80 - offset;
            // printf( "%d-%d: %d %d\n", ch, iii, offset, specification.voltageOffset[ ch ][ iii ] );
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // offset

    // HW gain, voltage steps in V/screenheight (ranges 20,50,100,200,500,1000,2000,5000 mV)
    // DDS120 has gainsteps 20x, 10x, 5x, 2x and 1x (as well as also 4x)
    // Hantek has only 10x, 5x, 2x, 1x
    // TODO: check if 20x is possible for ID 0 and ID 1
    // change 10 to 20 and double the values of specification.voltageLimit[] above
    specification.gain = {  // ID, HW gain, full scale voltage
        { 20,  0.16},        // 0     20         160mV =  20mV/div
        { 20,  0.40},        // 1     20         400mV =  50mV/div
        { 10,  0.80},        // 2     10         800mV = 100mV/div
        {  5,  1.60},        // 3      5          1.6V = 200mV/div
        {  2,  4.00},        // 4      2          4.0V = 500mV/div
        {  1,  8.00},        // 5      1          8.0V =    1V/div
        {  1, 16.00},        // 6      1         16.0V =    2V/div
        {  1, 40.00}         // 7      1         40.0V =    5V/div
    };


    // Possible raw sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 20k, 50k, 64k, 100k, 200k, 500k, 1M, 2M, 3M, 4M, 5M, 6M, 8M, 10M, 12M, 15M, 16M, 24M, 30M (, 48M)
    // 48M is unusable in 1 channel mode due to massive USB overrun
    // 24M, 30M and 48M are unusable in 2 channel mode
    // these unstable settings are disabled
    // Lower effective sample rates < 10 MS/s use oversampling to increase the SNR

    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.maxDownsampler = 10;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.maxDownsampler = 10;
    specification.samplerate.multi.recordLengths = { UINT_MAX };

// define VERY_SLOW_SAMPLES to get timebase up to 1s/div at the expense of very slow reaction time (up to 20 s)
//#define VERY_SLOW_SAMPLES
    specification.fixedSampleRates = { // samplerate, sampleId, downsampling
#ifdef VERY_SLOW_SAMPLES
        {  1e3, 110, 100}, // 100x downsampling from 100, 200, 500 kS/s!
        {  2e3, 120, 100}, //
        {  5e3, 150, 100}, //
#endif
        { 10e3,   1, 100}, // 100x downsampling from 1, 2, 5, 10 MS/s!
        { 20e3,   2, 100}, //
        { 50e3,   5, 100}, //
        {100e3,  10, 100}, //
        {200e3,  10,  50}, // 50x, 20x 10x, 5x, 2x downsampling from 10 MS/s
        {500e3,  10,  20}, //
        {  1e6,  10,  10}, //
        {  2e6,  10,   5}, //
        {  5e6,  10,   2}, //
        { 10e6,  10,   1}, // no oversampling
        { 12e6,  12,   1}, //
        { 15e6,  15,   1}, //
        { 24e6,  24,   1}, //
        { 30e6,  30,   1}, //
        { 48e6,  48,   1}  //
    };


    specification.couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    specification.triggerModes = {Dso::TriggerMode::AUTO, Dso::TriggerMode::NORMAL, Dso::TriggerMode::SINGLE};
    specification.fixedUSBinLength = 0;
    // calibration frequency (requires >FW0206)
    specification.calfreqSteps = { 50, 60, 100, 200, 500, 1e3, 2e3, 5e3, 10e3, 20e3, 50e3, 100e3 };
    specification.hasCalibrationStorage = false;
}

static void applyRequirements_(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new ControlSetVoltDIV_CH1());  // 0xE0
    dsoControl->addCommand(new ControlSetVoltDIV_CH2());  // 0xE1
    dsoControl->addCommand(new ControlSetTimeDIV());      // 0xE2
    dsoControl->addCommand(new ControlAcquireHardData()); // 0xE3
    dsoControl->addCommand(new ControlSetNumChannels());  // 0xE4
    dsoControl->addCommand(new ControlSetCoupling());     // 0xE5
    dsoControl->addCommand(new ControlSetCalFreq());      // 0xE6
}

//                                        VID/PID active  VID/PID no FW   FW ver  FW name  Scope name
//                                        |------------|  |------------|  |----|  |------|  |------|
ModelDDS120::ModelDDS120() : DSOModel(ID, 0x04b5, 0x0120, 0x8102, 0x8102, 0x0206, "dds120", "DDS120",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDDS120::applyRequirements(HantekDsoControl *dsoControl) const {
    applyRequirements_(dsoControl);
}

