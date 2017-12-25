#include "modelDSO5200.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

ModelDSO5200::ModelDSO5200() : DSOModel(ID, 0x04b5, 0x5200, 0x04b4, 0x5200, "dso5200x86", "DSO-5200", Hantek::ControlSpecification()) {
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.bulk.setGain = BULK_SETGAIN;
    specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
    specification.command.bulk.setChannels = BULK_ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setSamplerate = BULK_CSETTRIGGERORSAMPLERATE;
    specification.command.bulk.setTrigger = BULK_ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setPretrigger = BULK_ESETTRIGGERORSAMPLERATE;
    // specification.command.values.voltageLimits = VALUE_ETSCORRECTION;

    specification.samplerate.single.base = 100e6;
    specification.samplerate.single.max = 125e6;
    specification.samplerate.single.maxDownsampler = 131072;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 14336};
    specification.samplerate.multi.base = 200e6;
    specification.samplerate.multi.max = 250e6;
    specification.samplerate.multi.maxDownsampler = 131072;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 28672};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.gainSteps = { 0.16 , 0.40 , 0.80 , 1.60 , 4.00 , 8.0 , 16.0 , 40.0 , 80.0 };
    /// \todo Use calibration data to get the DSO-5200(A) sample ranges
    specification.voltageLimit[0] = { 368 , 454 , 908 , 368 , 454 , 908 , 368 , 454 , 908 };
    specification.voltageLimit[1] = { 368 , 454 , 908 , 368 , 454 , 908 , 368 , 454 , 908 };
    specification.gainIndex = { 1 , 0 , 0 , 1 , 0 , 0 , 1 , 0 , 0 };
    specification.sampleSize = 10;
}

void ModelDSO5200::applyRequirements(HantekDsoControl *dsoControl) const {
    // Instantiate additional commands for the DSO-5200
    dsoControl->command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetSamplerate5200();
    dsoControl->command[BULK_DSETBUFFER] = new BulkSetBuffer5200();
    dsoControl->command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetTrigger5200();
    dsoControl->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
    dsoControl->commandPending[BULK_DSETBUFFER] = true;
    dsoControl->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

    dsoControl->controlPending[CONTROLINDEX_SETOFFSET] = true;
    dsoControl->controlPending[CONTROLINDEX_SETRELAYS] = true;
}

ModelDSO5200A::ModelDSO5200A() {
    productID = 0x520a;
    productIDnoFirmware = 0x520a;
    firmwareToken = "dso5200ax86";
    name = "DSO-5200A";
}
