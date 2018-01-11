#include "modelDSO2250.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

ModelDSO2250::ModelDSO2250() : DSOModel(ID, 0x04b5, 0x2250, 0x04b4, 0x2250, "dso2250x86", "DSO-2250",
                                        Dso::ControlSpecification(2)) {
    specification.cmdSetRecordLength = BulkCode::DSETBUFFER;
    specification.cmdSetChannels = BulkCode::BSETCHANNELS;
    specification.cmdSetSamplerate = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.cmdSetTrigger = BulkCode::CSETTRIGGERORSAMPLERATE;
    specification.cmdSetPretrigger = BulkCode::FSETBUFFER;

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
    dsoControl->addCommand(new BulkForceTrigger(), false);
    dsoControl->addCommand(new BulkCaptureStart(), false);
    dsoControl->addCommand(new BulkTriggerEnabled(), false);
    dsoControl->addCommand(new BulkGetData(), false);
    dsoControl->addCommand(new BulkGetCaptureState(), false);
    dsoControl->addCommand(new BulkSetGain(), false);

    // Instantiate additional commands for the DSO-2250
    dsoControl->addCommand(new BulkSetChannels2250(), false);
    dsoControl->addCommand(new BulkSetTrigger2250(), false);
    dsoControl->addCommand(new BulkSetRecordLength2250(), false);
    dsoControl->addCommand(new BulkSetSamplerate2250(), false);
    dsoControl->addCommand(new BulkSetBuffer2250(), false);
    dsoControl->addCommand(new ControlSetOffset(), false);
    dsoControl->addCommand(new ControlSetRelays(), false);
}
