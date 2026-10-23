// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelRP2040 : public DSOModel {
    static const int ID = 0x2040;
    ModelRP2040();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};

