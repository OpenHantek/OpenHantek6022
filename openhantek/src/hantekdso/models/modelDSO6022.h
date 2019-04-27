#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

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
