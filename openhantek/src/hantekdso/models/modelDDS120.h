// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelDDS120 : public DSOModel {
    static const int ID = 0x0120;
    ModelDDS120();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};
