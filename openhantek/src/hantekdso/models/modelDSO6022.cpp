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
    // 6022xx do not support any bulk commands
    specification.useControlNoBulk = true;
    specification.isSoftwareTriggerDevice = true;
    specification.isFixedSamplerateDevice = true;
    specification.supportsCaptureState = false;
    specification.supportsOffset = false;
    specification.supportsCouplingRelays = false;
    // we drop 2k sample values due to unreliable start of stream
    // 20K samples at 100kS/s = 204,8 ms gives enough to fill 
    // the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
    const unsigned samples = (20 + 2) * 1024;
    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 30e6;
    specification.samplerate.single.maxDownsampler = 10;
    specification.samplerate.single.recordLengths = {UINT_MAX, samples};
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 16e6;
    specification.samplerate.multi.maxDownsampler = 10;
    specification.samplerate.multi.recordLengths = {UINT_MAX, samples * 2};
    specification.bufferDividers = { 1000 , 1 , 1 };
    // This data was based on testing and depends on Divider.
    // The sample value at the top of the screen
    specification.voltageLimit[0] = { 20 , 40 , 100 , 200 , 200 , 200 , 400 , 400 , 1000 };
    specification.voltageLimit[1] = { 20 , 40 , 100 , 200 , 200 , 200 , 400 , 400 , 1000 };
    specification.voltageOffset[0] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    specification.voltageOffset[1] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    const char* ranges[] = { "10mV", "20mV", "50mV","100mV", "200mV", "500mV", "1000mV", "2000mV", "5000mV" }; 
    const char* channels[] = { "ch0", "ch1" };

    //HORO: read the real calibration values from file
    const unsigned RANGES = 9;
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
            int offset = settings.value( ranges[ iii ], "0" ).toInt();
            if ( offset ) 
                specification.voltageOffset[ ch ][ iii ] -= offset;
        }
        settings.endGroup(); // channels
    }
    settings.endGroup(); // offset

    // Divider. Tested and calculated results are different!
    // HW gain, voltage steps in V/screenheight (ranges 10,20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = { {10,0.08} , {10,0.16} , {10,0.40} , {10,0.80} , {5,1.60} ,
                           {2,4.00} , {2,8.00} , {1,16.00} , {1,40.00} };
    // Sample rates with default fw (fw modded from https://github.com/jhoenicke/Hantek6022API)
    // 100k, 200k, 500k, 1M, 2M, 4M, 8M, 12M, 16M, 24M, 30M, 48M, (30M & 48M are unstable with 2 channels)
    specification.fixedSampleRates = { {10,1e5} , {20,2e5} , {50,5e5} , {1,1e6} , {2,2e6} , {4,4e6} , {8,8e6} ,
                                       {12,12e6} , {16,16e6} , {24,24e6} , {30,30e6} /*, {48,48e6}*/ }; // 30&48 are unstable
    specification.sampleSize = 8;

    specification.couplings = {Dso::Coupling::DC};
    specification.triggerModes = {Dso::TriggerMode::HARDWARE_SOFTWARE, Dso::TriggerMode::SINGLE};
    specification.fixedUSBinLength = 16384;
}

void applyRequirements_(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new ControlAcquireHardData());
    dsoControl->addCommand(new ControlSetTimeDIV());
    dsoControl->addCommand(new ControlSetVoltDIV_CH2());
    dsoControl->addCommand(new ControlSetVoltDIV_CH1());
    dsoControl->addCommand(new ControlSetNumChannels());
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
