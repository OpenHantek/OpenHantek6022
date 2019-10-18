#include <QDir>
#include <QSettings>
#include "modelDSO6022.h"
#include "usb/usbdevice.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

#define VERBOSE 0

using namespace Hantek;

static ModelDSO2020 modelInstance_20;
static ModelDSO6022BE modelInstance_22;
static ModelDSO6022BL modelInstance_2a;
#ifdef LCSOFT_TEST_BOARD
// two test cases with simple EZUSB board (LCsoft) without EEPROM or with Saleae VID/PID EEPROM
static ModelEzUSB modelInstance4;
static ModelSaleae modelInstance5;
#endif

static void initSpecifications(Dso::ControlSpecification& specification) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in modelDSO6022.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()
    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.maxDownsampler = 10;
    specification.samplerate.single.recordLengths = { UINT_MAX };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 15e6;
    specification.samplerate.multi.maxDownsampler = 10;
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
    specification.voltageOffset[0] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    specification.voltageOffset[1] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // read the real calibration values from file
    const char* ranges[] = { "20mV", "50mV","100mV", "200mV", "500mV", "1000mV", "2000mV", "5000mV" }; 
    const char* channels[] = { "ch0", "ch1" };
    //printf( "read config file\n" );
    const unsigned RANGES = 8;
    QSettings settings( QDir::homePath() + "/.config/OpenHantek/modelDSO6022.conf", QSettings::IniFormat );

    settings.beginGroup( "gain" );
    for ( unsigned ch = 0; ch < 2; ch++ ) {
        settings.beginGroup( channels[ ch ] );
        for ( unsigned iii = 0; iii < RANGES; iii++ ) {
            double calibration = settings.value( ranges[ iii ], "0.0" ).toDouble();
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
            //printf( "%d: %d\n", iii, offset );
            if ( offset != 255 ) // value exists in config file
                specification.voltageOffset[ ch ][ iii ] = 0x80 - offset;
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // offset

    // HW gain, voltage steps in V/screenheight (ranges 20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = {
        {10,0.16} , {10,0.40} , {10,0.80} , {5,1.60} ,
        {2,4.00} , {1,8.00} , {1,16.00} , {1,40.00}
    };

    // Possible sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 60k, 100k, 200k, 500k, 1M, 2M, 3M, 4M, 5M, 6M, 8M, 10M, 12M, 15M, 16M, 24M, 30M (, 48M)
    // 48M is unstable in 1 channel mode
    // 24M, 30M and 48M are unstable in 2 channel mode
// define VERY_SLOW_SAMPLES to get timebase up to 1s/div at the expense of very slow reaction time (up to 20 s)
//#define VERY_SLOW_SAMPLES
    specification.fixedSampleRates = { // samplerate, sampleId, downsampling
#ifdef VERY_SLOW_SAMPLES
        {  1e3, 110, 100} , {  2e3, 120, 100} , { 5e3, 150, 100} , // massive downsampling from 100, 200, 500 kS/s!
#endif
        { 10e3,   1, 100} , { 20e3,   2, 100} , { 50e3,   5, 100} , // 100x downsampling from 1, 2, 5 MS/s!
        {100e3,  10, 100} , {200e3,  10,  50} , {500e3,  10,  20} , // 100x, 50x, 20x downsampling from 10 MS/s
        {  1e6,  10,  10} , {  2e6,  10,   5} , {  5e6,  10,   2} , // 10x,   5x,  2x downsampling from 10 MS/s
        { 10e6,  10,   1} , { 12e6,  12,   1} , { 15e6,  15,   1} , // no oversampling
        { 24e6,  24,   1} , { 30e6,  30,   1}  // no oversampling
    };

    specification.sampleSize = specification.fixedSampleRates.size();
    specification.couplings = {Dso::Coupling::DC};
    //specification.couplings = {Dso::Coupling::DC, Dso::Coupling::AC}; // requires AC/DC HW mod like DDS120
    specification.triggerModes = {Dso::TriggerMode::AUTO, Dso::TriggerMode::NORMAL, Dso::TriggerMode::SINGLE};
    specification.fixedUSBinLength = 0;
}

static void applyRequirements_(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new ControlSetVoltDIV_CH1());  // 0xE0
    dsoControl->addCommand(new ControlSetVoltDIV_CH2());  // 0xE1
    dsoControl->addCommand(new ControlSetTimeDIV());      // 0xE2
    dsoControl->addCommand(new ControlAcquireHardData()); // 0xE3
    dsoControl->addCommand(new ControlSetNumChannels());  // 0xE4
    dsoControl->addCommand(new ControlSetCoupling());     // 0xE5 (no effect w/o AC/DC HW mod)
    dsoControl->addCommand(new ControlSetCalFreq());      // 0xE6
}

//                                              VID/PID active  VID/PID no FW   FW ver    FW name     Scope name
//                                              |------------|  |------------|  |----|  |---------|  |----------|
ModelDSO6022BE::ModelDSO6022BE() : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x6022, 0x0203, "dso6022be", "DSO-6022BE",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BE::applyRequirements(HantekDsoControl *dsoControl) const {
    applyRequirements_(dsoControl);
}

// Hantek DSO-6022BL (scope or logic analyzer)
ModelDSO6022BL::ModelDSO6022BL() : DSOModel(ID, 0x04b5, 0x602a, 0x04b4, 0x602a, 0x0203, "dso6022bl", "DSO-6022BL",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BL::applyRequirements(HantekDsoControl *dsoControl) const {
   applyRequirements_(dsoControl);
}

// Voltcraft DSO-2020 USB Oscilloscope
// Scope starts up as model DS-2020 (VID/PID = 04b4/2020) but loads 6022BE firmware and looks like a 6022BE 
ModelDSO2020::ModelDSO2020() : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x2020, 0x0203, "dso6022be", "DSO-2020",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO2020::applyRequirements(HantekDsoControl *dsoControl) const {
    applyRequirements_(dsoControl);
}


#ifdef LCSOFT_TEST_BOARD
// two test cases with simple EZUSB board (LCsoft) without EEPROM or with Saleae VID/PID EEPROM
// after loading the FW they look like a 6022BE (without useful sample values as Port B and D are left open)
ModelEzUSB::ModelEzUSB() : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x8613, 0x0203, "dso6022be", "LCsoft-EzUSB",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelEzUSB::applyRequirements(HantekDsoControl *dsoControl) const {
   applyRequirements_(dsoControl);
}

ModelSaleae::ModelSaleae() : DSOModel(ID, 0x04b5, 0x6022, 0x0925, 0x3881, 0x0203, "dso6022be", "LCsoft-Saleae",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelSaleae::applyRequirements(HantekDsoControl *dsoControl) const {
   applyRequirements_(dsoControl);
}
#endif
