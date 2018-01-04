#include "modelDSO5200.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

ModelDSO5200::ModelDSO5200() : DSOModel(ID, 0x04b5, 0x5200, 0x04b4, 0x5200, "dso5200x86", "DSO-5200", Dso::ControlSpecification()) {
    specification.command.bulk.setRecordLength = BulkCode::DSETBUFFER;
    specification.command.bulk.setChannels = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setSamplerate = BulkCode::CSETTRIGGERORSAMPLERATE;
    specification.command.bulk.setTrigger = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setPretrigger = BulkCode::ESETTRIGGERORSAMPLERATE;
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
    /// \todo Use calibration data to get the DSO-5200(A) sample ranges
    specification.voltageLimit[0] = { 368 , 454 , 908 , 368 , 454 , 908 , 368 , 454 , 908 };
    specification.voltageLimit[1] = { 368 , 454 , 908 , 368 , 454 , 908 , 368 , 454 , 908 };
    specification.gain = { {1,0.16} , {0,0.40} , {0,0.80} , {1,1.60} ,
                           {0,4.00} , {0,8.00} , {1,16.0} , {0,40.0} , {0,80.0} };
    specification.sampleSize = 10;
    specification.specialTriggerChannels = {{"EXT", -2}, {"EXT/10", -3}}; // 3, 4
}

void ModelDSO5200::applyRequirements(HantekDsoControl *dsoControl) const {
    // Instantiate additional commands for the DSO-5200
    dsoControl->addCommand(BulkCode::CSETTRIGGERORSAMPLERATE, new BulkSetSamplerate5200());
    dsoControl->addCommand(BulkCode::DSETBUFFER, new BulkSetBuffer5200());
    dsoControl->addCommand(BulkCode::ESETTRIGGERORSAMPLERATE, new BulkSetTrigger5200());
    dsoControl->addCommand(ControlCode::CONTROL_SETOFFSET, new ControlSetOffset());
    dsoControl->addCommand(ControlCode::CONTROL_SETRELAYS, new ControlSetRelays());
}

ModelDSO5200A::ModelDSO5200A() {
    productID = 0x520a;
    productIDnoFirmware = 0x520a;
    firmwareToken = "dso5200ax86";
    name = "DSO-5200A";
}
