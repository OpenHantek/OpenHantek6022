// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelDSO6021 : public DSOModel {
    static const int ID = 0x6021;
    ModelDSO6021();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};
