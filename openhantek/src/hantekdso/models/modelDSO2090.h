#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

struct ModelDSO2090 : public DSOModel {
    static const int ID = 0x2090;
    ModelDSO2090();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};

struct ModelDSO2090A : public ModelDSO2090 {
    ModelDSO2090A();
};

