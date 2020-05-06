// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelDEMO : public DSOModel {
    static const int ID = 0xDEDE;
    ModelDEMO();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};
