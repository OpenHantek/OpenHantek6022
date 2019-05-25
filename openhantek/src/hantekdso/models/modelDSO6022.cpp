#include <QDir>
#include <QSettings>
#include "modelDSO6022.h"
#include "usb/usbdevice.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

#define VERBOSE 0

using namespace Hantek;

static ModelDSO6022BE modelInstance;
static ModelDSO6022BL modelInstance2;

static void initSpecifications(Dso::ControlSpecification& specification) {
    // we drop 2K + 480 sample values due to unreliable start of stream
    // 20000 samples at 100kS/s = 200 ms gives enough to fill
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    // SAMPLESIZE defined in modelDSO6022.h
    // adapt accordingly in HantekDsoControl::convertRawDataToSamples()
    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.maxDownsampler = 10;
    specification.samplerate.single.recordLengths = {
        UINT_MAX, SAMPLESIZE_RAW, SAMPLESIZE_RAW_L, SAMPLESIZE_RAW_XL, SAMPLESIZE_RAW_XXL
    };
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 16e6;
    specification.samplerate.multi.maxDownsampler = 10;
    specification.samplerate.multi.recordLengths = {
        UINT_MAX, SAMPLESIZE_RAW * 2, SAMPLESIZE_RAW_L * 2,
        SAMPLESIZE_RAW_XL * 2, SAMPLESIZE_RAW_XXL * 2
    };
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

    // HW gain, voltage steps in V/screenheight (ranges 10,20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = { {10,0.16} , {10,0.40} , {10,0.80} , {5,1.60} ,
                           {2,4.00} , {1,8.00} , {1,16.00} , {1,40.00} };
    // Sample rates with custom fw from https://github.com/Ho-Ro/Hantek6022API
    // 100k, 200k, 500k, 1M, 2M, 4M, 8M, 12M, 16M, 24M, 30M, 48M
    // 48M is unstable in 1 channel mode
    // 24M, 30M and 48M are unstable in 2 channel mode
    specification.fixedSampleRates = { {10,1e5} , {20,2e5} , {50,5e5} , {1,1e6} , {2,2e6} , {4,4e6} , 
                                       {8,8e6} , {12,12e6} , {16,16e6} , {24,24e6} , {30,30e6} }; 
    specification.sampleSize = 8;

    specification.couplings = {Dso::Coupling::DC};
    specification.triggerModes = {Dso::TriggerMode::AUTO, Dso::TriggerMode::NORMAL, Dso::TriggerMode::SINGLE};
    specification.fixedUSBinLength = SAMPLESIZE_RAW;
}

void applyRequirements_(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new ControlAcquireHardData());
    dsoControl->addCommand(new ControlSetTimeDIV());
    dsoControl->addCommand(new ControlSetVoltDIV_CH2());
    dsoControl->addCommand(new ControlSetVoltDIV_CH1());
    dsoControl->addCommand(new ControlSetNumChannels());
    dsoControl->addCommand(new ControlSetCalFreq());
}

ModelDSO6022BE::ModelDSO6022BE() : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x6022, "dso6022be", "DSO-6022BE",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BE::applyRequirements(HantekDsoControl *dsoControl) const {
    applyRequirements_(dsoControl);
}

ModelDSO6022BL::ModelDSO6022BL() : DSOModel(ID, 0x04b5, 0x602a, 0x04b4, 0x602a, "dso6022bl", "DSO-6022BL",
                                            Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BL::applyRequirements(HantekDsoControl *dsoControl) const {
   applyRequirements_(dsoControl);
}
