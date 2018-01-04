#include "modelDSO2150.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

ModelDSO2150::ModelDSO2150() : DSOModel(ID, 0x04b5, 0x2150, 0x04b4, 0x2150, "dso2150x86", "DSO-2150", Dso::ControlSpecification()) {
    specification.command.bulk.setRecordLength = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setChannels = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setSamplerate = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setTrigger = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setPretrigger = BulkCode::SETTRIGGERANDSAMPLERATE;

    specification.samplerate.single.base = 50e6;
    specification.samplerate.single.max = 75e6;
    specification.samplerate.single.maxDownsampler = 131072;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 32768};
    specification.samplerate.multi.base = 100e6;
    specification.samplerate.multi.max = 150e6;
    specification.samplerate.multi.maxDownsampler = 131072;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 65536};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.voltageLimit[0] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.voltageLimit[1] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.gain = { {0,0.08} , {1,0.16} , {2,0.40} , {0,0.80} ,
                           {1,1.60} , {2,4.00} , {0,8.00} , {1,16.00} , {2,40.00} };
    specification.sampleSize = 8;
    specification.specialTriggerChannels = {{"EXT", -2}, {"EXT/10", -3}};
}

void ModelDSO2150::applyRequirements(HantekDsoControl *dsoControl) const {
    dsoControl->addCommand(BulkCode::SETTRIGGERANDSAMPLERATE, new BulkSetTriggerAndSamplerate());
    dsoControl->addCommand(ControlCode::CONTROL_SETOFFSET, new ControlSetOffset());
    dsoControl->addCommand(ControlCode::CONTROL_SETRELAYS, new ControlSetRelays());
}
