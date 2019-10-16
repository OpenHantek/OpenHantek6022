#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

#define SAMPLESIZE_USED 20000


struct ModelDDS120 : public DSOModel {
    static const int ID = 0x0120;
    ModelDDS120();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};

