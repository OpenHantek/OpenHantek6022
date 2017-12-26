#include "modelDSO2250.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

ModelDSO2250::ModelDSO2250() : DSOModel(ID, 0x04b5, 0x2250, 0x04b4, 0x2250, "dso2250x86", "DSO-2250", Hantek::ControlSpecification()) {
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.bulk.setGain = BulkCode::SETGAIN;
    specification.command.bulk.setRecordLength = BulkCode::DSETBUFFER;
    specification.command.bulk.setChannels = BulkCode::BSETCHANNELS;
    specification.command.bulk.setSamplerate = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.command.bulk.setTrigger = BulkCode::CSETTRIGGERORSAMPLERATE;
    specification.command.bulk.setPretrigger = BulkCode::FSETBUFFER;

    specification.samplerate.single.base = 100e6;
    specification.samplerate.single.max = 100e6;
    specification.samplerate.single.maxDownsampler = 65536;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 524288};
    specification.samplerate.multi.base = 200e6;
    specification.samplerate.multi.max = 250e6;
    specification.samplerate.multi.maxDownsampler = 65536;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 1048576};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.voltageLimit[0] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.voltageLimit[1] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.gain = { {0,0.08} , {2,0.16} , {3,0.40} , {0,0.80} ,
                           {2,1.60} , {3,4.00} , {0,8.00} , {2,16.00} , {3,40.00} };
    specification.sampleSize = 8;
    specification.specialTriggerChannels = {{"EXT", -2}};
}

void ModelDSO2250::applyRequirements(HantekDsoControl *dsoControl) const {
    // Instantiate additional commands for the DSO-2250
    dsoControl->command[(uint8_t)BulkCode::BSETCHANNELS] = new BulkSetChannels2250();
    dsoControl->command[(uint8_t)BulkCode::CSETTRIGGERORSAMPLERATE] = new BulkSetTrigger2250();
    dsoControl->command[(uint8_t)BulkCode::DSETBUFFER] = new BulkSetRecordLength2250();
    dsoControl->command[(uint8_t)BulkCode::ESETTRIGGERORSAMPLERATE] = new BulkSetSamplerate2250();
    dsoControl->command[(uint8_t)BulkCode::FSETBUFFER] = new BulkSetBuffer2250();
    dsoControl->commandPending[(uint8_t)BulkCode::BSETCHANNELS] = true;
    dsoControl->commandPending[(uint8_t)BulkCode::CSETTRIGGERORSAMPLERATE] = true;
    dsoControl->commandPending[(uint8_t)BulkCode::DSETBUFFER] = true;
    dsoControl->commandPending[(uint8_t)BulkCode::ESETTRIGGERORSAMPLERATE] = true;
    dsoControl->commandPending[(uint8_t)BulkCode::FSETBUFFER] = true;

    dsoControl->controlPending[CONTROLINDEX_SETOFFSET] = true;
    dsoControl->controlPending[CONTROLINDEX_SETRELAYS] = true;
}
