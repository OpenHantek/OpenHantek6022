#include "modelDSO2250.h"

using namespace Hantek;

ModelDSO2250::ModelDSO2250() : DSOModel(ID, 0x04b5, 0x2250, 0x04b4, 0x2250, "dso2250x86", "DSO-2250", Hantek::ControlSpecification()) {
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.bulk.setGain = BULK_SETGAIN;
    specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
    specification.command.bulk.setChannels = BULK_BSETCHANNELS;
    specification.command.bulk.setSamplerate = BULK_ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setTrigger = BULK_CSETTRIGGERORSAMPLERATE;
    specification.command.bulk.setPretrigger = BULK_FSETBUFFER;

    specification.samplerate.single.base = 100e6;
    specification.samplerate.single.max = 100e6;
    specification.samplerate.single.maxDownsampler = 65536;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 524288};
    specification.samplerate.multi.base = 200e6;
    specification.samplerate.multi.max = 250e6;
    specification.samplerate.multi.maxDownsampler = 65536;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 1048576};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.gainSteps = { 0.08 , 0.16 , 0.40 , 0.80 , 1.60 , 4.00 , 8.0 , 16.0 , 40.0 };
    specification.voltageLimit[0] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.voltageLimit[1] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.gainIndex = { 0 , 2 , 3 , 0 , 2 , 3 , 0 , 2 , 3 };
    specification.sampleSize = 8;
}

void ModelDSO2250::applyRequirements(HantekDsoControl *dsoControl) const {
    // Instantiate additional commands for the DSO-2250
    dsoControl->command[BULK_BSETCHANNELS] = new BulkSetChannels2250();
    dsoControl->command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetTrigger2250();
    dsoControl->command[BULK_DSETBUFFER] = new BulkSetRecordLength2250();
    dsoControl->command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetSamplerate2250();
    dsoControl->command[BULK_FSETBUFFER] = new BulkSetBuffer2250();
    dsoControl->commandPending[BULK_BSETCHANNELS] = true;
    dsoControl->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
    dsoControl->commandPending[BULK_DSETBUFFER] = true;
    dsoControl->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
    dsoControl->commandPending[BULK_FSETBUFFER] = true;

    dsoControl->controlPending[CONTROLINDEX_SETOFFSET] = true;
    dsoControl->controlPending[CONTROLINDEX_SETRELAYS] = true;
}
