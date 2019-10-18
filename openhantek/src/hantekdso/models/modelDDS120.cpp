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
    specification.samplerate.single.max = 48e6;
    specification.samplerate.single.maxDownsampler = 100;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.maxDownsampler = 100;
    specification.samplerate.multi.recordLengths = { UINT_MAX };
    specification.bufferDividers = { 1000 , 1 , 1 };
    // This data was based on testing and depends on Divider.
    // Input divider: 100/1009 = 1% too low display
    // Amplifier gain: x1 (ok), x2 (ok), x5.1 (2% too high), x10.1 (1% too high)
    // Overall gain: x1 1% too low, x2 1% to low, x5 1% to high, x10 ok
    // The sample value at the top of the screen with gain error correction
    specification.voltageLimit[0] = { 40 , 100 , 200 , 202 , 198 , 198 , 396 , 990 };
    specification.voltageLimit[1] = { 40 , 100 , 200 , 202 , 198 , 198 , 396 , 990 };
    // theoretical offset, will be corrected by individual config file
    specification.voltageOffset[0] = { 136, 136, 136, 136, 136, 136, 136, 136 };
    specification.voltageOffset[1] = { 132, 132, 132, 132, 132, 132, 132, 132 };

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
            double calibration = settings.value( ranges[ iii ], ch == 0 ? 1.18 : 1.21 ).toDouble();
            if ( calibration )
                specification.voltageLimit[ ch ][ iii ] /= calibration;
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
    specification.gain = {  // ID, HW gain, full scale voltage
        { 10,  0.16},        // 0     10         160mV
        { 10,  0.40},        // 1     10         400mV
        { 10,  0.80},        // 2     10         800mV
        {  5,  1.60},        // 3      5          1.6V
        {  2,  4.00},        // 4      2          4.0V
        {  1,  8.00},        // 5      1          8.0V
        {  1, 16.00},        // 6      1         16.0V
        {  1, 40.00}         // 7      1         40.0V
    };

    // Possible sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 60k, 100k, 200k, 500k, 1M, 2M, 3M, 4M, 5M, 6M, 8M, 10M, 12M, 15M, 16M, 24M, 30M (, 48M)
    // 48M is unstable in 1 channel mode
    // 24M, 30M and 48M are unstable in 2 channel mode
// define VERY_SLOW_SAMPLES to get timebase up to 1 s/div at the expense of very slow reaction time (up to 20 s)
//#define VERY_SLOW_SAMPLES
    specification.fixedSampleRates = { 
    // samplerate, sampleId, downsampling
#ifdef VERY_SLOW_SAMPLES
        {  1e3, 110, 100}, 
        {  2e3, 120, 100}, 
        {  5e3, 150, 100}, // massive downsampling from 100, 200, 500 kS/s!
#endif
        { 10e3,   1, 100}, // 100x downsampling from  1 MS/s!
        { 20e3,   2, 100}, // 100x downsampling from  2 MS/s!
        { 50e3,   5, 100}, // 100x downsampling from  5 MS/s!
        {100e3,   8,  80}, // 100x downsampling from  8 MS/s
        {200e3,   8,  40}, //  50x downsampling from  8 MS/s
        {500e3,   8,  16}, //  20x downsampling from  8 MS/s
        {  1e6,   8,   8}, //   8x downsampling from  8 MS/s
        {  2e6,   8,   4}, //   4x downsampling from  8 MS/s
        {  5e6,  15,   3}, //   3x downsampling from 15 MS/s
        // { 10e6,  30,   3}, //   3x downsampling from 30 MS/s
        { 15e6,  15,   1}, // no downsampling
        { 30e6,  30,   1}, // no downsampling
        { 48e6,  48,   1}  // no downsampling
    };

    specification.sampleSize = specification.fixedSampleRates.size();
    specification.couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    specification.triggerModes = {Dso::TriggerMode::AUTO, Dso::TriggerMode::NORMAL, Dso::TriggerMode::SINGLE};
    specification.fixedUSBinLength = 0;
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

//                                        VID/PID active  VID/PID no FW   FW ver  FW name   Scope name
//                                        |------------|  |------------|  |----|  |------|  |--------|
ModelDDS120::ModelDDS120() : DSOModel(ID, 0x1d50, 0x608e, 0x8102, 0x8102, 0x02,   "dds120", "DDS120",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDDS120::applyRequirements(HantekDsoControl *dsoControl) const {
    applyRequirements_(dsoControl);
}

