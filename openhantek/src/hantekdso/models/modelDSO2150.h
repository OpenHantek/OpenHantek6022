#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

struct ModelDSO2150 : public DSOModel {
    static const int ID = 0x2150;
    ModelDSO2150();
    virtual void applyRequirements(HantekDsoControl* dsoControl) const override;
};
