#include "modelDSO6022.h"
#include "usb/usbdevice.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

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

    specification.samplerate.single.base = 1e6;
    specification.samplerate.single.max = 48e6;
    specification.samplerate.single.maxDownsampler = 10;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240};
    specification.samplerate.multi.base = 1e6;
    specification.samplerate.multi.max = 48e6;
    specification.samplerate.multi.maxDownsampler = 10;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480};
    specification.bufferDividers = { 1000 , 1 , 1 };
    // This data was based on testing and depends on Divider.
    // The sample value at the top of the screen
    specification.voltageLimit[0] = { 20 , 40 , 101 , 202 , 404 , 208 , 410 , 410 , 1060 };
    specification.voltageLimit[1] = { 20 , 40 , 100 , 200 , 400 , 208 , 410 , 410 , 1060 };
    // Divider. Tested and calculated results are different!
    // HW gain, voltage steps in V/screenheight (ranges 10,20,50,100,200,500,1000,2000,5000 mV)
    specification.gain = { {10,0.08} , {10,0.16} , {10,0.40} , {10,0.80} , {10,1.60} ,
                           {2,4.00} , {2,8.00} , {1,16.00} , {1,40.00} };
    specification.fixedSampleRates = { {10,1e5} , {20,2e5} , {50,5e5} , {1,1e6} , {2,2e6} , {4,4e6} , {8,8e6} ,
                                       {16,16e6} , {24,24e6} , {48,48e6} };
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
