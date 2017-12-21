#include "modelDSO2090.h"

using namespace Hantek;

ModelDSO2090::ModelDSO2090() : DSOModel(ID, 0x04b5, 0x2090, 0x04b4, 0x2090, "dso2090x86", "DSO-2090", Hantek::ControlSpecification()) {
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.bulk.setGain = BULK_SETGAIN;
    specification.command.bulk.setRecordLength = BULK_SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setChannels = BULK_SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setPretrigger = BULK_SETTRIGGERANDSAMPLERATE;

    specification.samplerate.single.base = 50e6;
    specification.samplerate.single.max = 50e6;
    specification.samplerate.single.maxDownsampler = 131072;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 32768};
    specification.samplerate.multi.base = 100e6;
    specification.samplerate.multi.max = 100e6;
    specification.samplerate.multi.maxDownsampler = 131072;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 65536};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.gainSteps = { 0.08 , 0.16 , 0.40 , 0.80 , 1.60 , 4.00 , 8.0 , 16.0 , 40.0 };
    specification.voltageLimit[0] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.voltageLimit[1] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.gainIndex = { 0 , 1 , 2 , 0 , 1 , 2 , 0 , 1 , 2 };
    specification.sampleSize = 8;
}

void ModelDSO2090::applyRequirements(HantekDsoControl *dsoControl)  const {
    dsoControl->command[BULK_SETTRIGGERANDSAMPLERATE] = new BulkSetTriggerAndSamplerate();
    dsoControl->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

    dsoControl->controlPending[CONTROLINDEX_SETOFFSET] = true;
    dsoControl->controlPending[CONTROLINDEX_SETRELAYS] = true;
}

ModelDSO2090A::ModelDSO2090A() {
    productIDnoFirmware = 0x8613;
}
