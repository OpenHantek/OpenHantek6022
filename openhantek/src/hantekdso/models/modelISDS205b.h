// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelISDS205B : public DSOModel {
    static const int ID = 0x2050;
    ModelISDS205B();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};
