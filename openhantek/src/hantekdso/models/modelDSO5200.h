#pragma once

#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "bulkStructs.h"

using namespace Hantek;

struct ModelDSO5200 : public DSOModel {
    static const int ID = 0x5200;
    ModelDSO5200();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};

struct ModelDSO5200A : public ModelDSO5200 {
    ModelDSO5200A();
};
