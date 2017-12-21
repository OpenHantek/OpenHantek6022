#pragma once

#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "bulkStructs.h"

using namespace Hantek;

struct ModelDSO2250 : public DSOModel {
    static const int ID = 0x2250;
    ModelDSO2250();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};
