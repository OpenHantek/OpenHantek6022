#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

// we drop 2K + 480 sample values due to unreliable start of stream
// 20000 samples at 100kS/s = 200 ms gives enough to fill
// the screen two times (for pre/post trigger) at 10ms/div = 100ms/screen
// adapt accordingly in HantekDsoControl::convertRawDataToSamples()
#define K_SAMPLES 20
#define SAMPLESIZE_RAW  (K_SAMPLES * 1024 + 2048)
#define SAMPLESIZE_USED (K_SAMPLES * 1000)
// The 1st two or three frames (512 byte) of the raw sample stream are unreliable
// (Maybe because the common mode input voltage of ADC is handled far out of spec and has to settle)
// Solution: drop (2048 + 480) heading samples from (22 * 1024) total samples
// 22 * 1024 - 2048 - 480 = 20000
#define DROP_DSO6022_HEAD ( SAMPLESIZE_RAW - SAMPLESIZE_USED )

struct ModelDSO6022BE : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO6022BE();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};

struct ModelDSO6022BL : public DSOModel {
    static const int ID = 0x602a;
    ModelDSO6022BL();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};
