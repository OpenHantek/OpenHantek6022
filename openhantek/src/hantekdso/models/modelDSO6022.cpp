#include "modelDSO6022.h"

using namespace Hantek;

ModelDSO6022BE::ModelDSO6022BE() : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x6022, "dso6022be", "DSO-6022BE", Hantek::ControlSpecification()) {
    // 6022BE do not support any bulk commands
    specification.useControlNoBulk = true;
    specification.isSoftwareTriggerDevice = true;
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
    specification.gainSteps = { 0.08 , 0.16 , 0.40 , 0.80 , 1.60 , 4.00 , 8.0 , 16.0 , 40.0 };
    // This data was based on testing and depends on Divider.
    specification.voltageLimit[0] = { 25 , 51 , 103 , 206 , 412 , 196 , 392 , 784 , 1000 };
    specification.voltageLimit[1] = { 25 , 51 , 103 , 206 , 412 , 196 , 392 , 784 , 1000 };
    // Divider. Tested and calculated results are different!
    specification.gainDiv = { 10 , 10 , 10 , 10 , 10 , 2 , 2 , 2 , 1 };
    specification.sampleSteps = { 1e5 , 2e5 , 5e5 , 1e6 , 2e6 , 4e6 , 8e6 , 16e6 , 24e6 , 48e6 };
    specification.sampleDiv = { 10 , 20 , 50 , 1 , 2 , 4 , 8 , 16 , 24 , 48 };
    specification.sampleSize = 8;
}

void ModelDSO6022BE::applyRequirements(HantekDsoControl *dsoControl) const {
    dsoControl->getDevice()->overwriteInPacketLength(16384);

    dsoControl->control[CONTROLINDEX_SETVOLTDIV_CH1] = new ControlSetVoltDIV_CH1();
    dsoControl->controlCode[CONTROLINDEX_SETVOLTDIV_CH1] = CONTROL_SETVOLTDIV_CH1;
    dsoControl->controlPending[CONTROLINDEX_SETVOLTDIV_CH1] = true;

    dsoControl->control[CONTROLINDEX_SETVOLTDIV_CH2] = new ControlSetVoltDIV_CH2();
    dsoControl->controlCode[CONTROLINDEX_SETVOLTDIV_CH2] = CONTROL_SETVOLTDIV_CH2;
    dsoControl->controlPending[CONTROLINDEX_SETVOLTDIV_CH2] = true;

    dsoControl->control[CONTROLINDEX_SETTIMEDIV] = new ControlSetTimeDIV();
    dsoControl->controlCode[CONTROLINDEX_SETTIMEDIV] = CONTROL_SETTIMEDIV;
    dsoControl->controlPending[CONTROLINDEX_SETTIMEDIV] = true;

    dsoControl->control[CONTROLINDEX_ACQUIIRE_HARD_DATA] = new ControlAcquireHardData();
    dsoControl->controlCode[CONTROLINDEX_ACQUIIRE_HARD_DATA] = CONTROL_ACQUIIRE_HARD_DATA;
    dsoControl->controlPending[CONTROLINDEX_ACQUIIRE_HARD_DATA] = true;

    dsoControl->controlPending[CONTROLINDEX_SETOFFSET] = false;
    dsoControl->controlPending[CONTROLINDEX_SETRELAYS] = false;
}

ModelDSO6022LE::ModelDSO6022LE() {
    productID = 0x602a;
    productIDnoFirmware = 0x602a;
    firmwareToken = "dso6022be";
    name = "DSO-6022LE";
}
