#include "modelDSO5200.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

static ModelDSO5200 modelInstance;
static ModelDSO5200A modelInstance2;

static void initSpecifications(Dso::ControlSpecification& specification) {
    specification.cmdSetRecordLength = BulkCode::DSETBUFFER;
    specification.cmdSetChannels = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.cmdSetSamplerate = BulkCode::CSETTRIGGERORSAMPLERATE;
    specification.cmdSetTrigger = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification.cmdSetPretrigger = BulkCode::ESETTRIGGERORSAMPLERATE;

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

static void _applyRequirements(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new BulkForceTrigger(), false);
    dsoControl->addCommand(new BulkCaptureStart(), false);
    dsoControl->addCommand(new BulkTriggerEnabled(), false);
    dsoControl->addCommand(new BulkGetData(), false);
    dsoControl->addCommand(new BulkGetCaptureState(), false);
    dsoControl->addCommand(new BulkSetGain(), false);

    // Instantiate additional commands for the DSO-5200
    dsoControl->addCommand(new BulkSetSamplerate5200(), false);
    dsoControl->addCommand(new BulkSetBuffer5200(), false);
    dsoControl->addCommand(new BulkSetTrigger5200(), false);
    dsoControl->addCommand(new ControlSetOffset(), false);
    dsoControl->addCommand(new ControlSetRelays(), false);
}

ModelDSO5200::ModelDSO5200() : DSOModel(ID, 0x04b5, 0x5200, 0x04b4, 0x5200, "dso5200x86", "DSO-5200",
                                        Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO5200::applyRequirements(HantekDsoControl *dsoControl) const {
    _applyRequirements(dsoControl);
}

ModelDSO5200A::ModelDSO5200A() : DSOModel(ID, 0x04b5, 0x520a, 0x04b4, 0x520a, "dso5200ax86", "DSO-5200A",
                                          Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO5200A::applyRequirements(HantekDsoControl *dsoControl) const {
    _applyRequirements(dsoControl);
}
