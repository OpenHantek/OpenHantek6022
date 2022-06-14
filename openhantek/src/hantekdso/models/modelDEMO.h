// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;

const int DemoDeviceID = 0xDEDE;

struct ModelDEMO : public DSOModel {
    static const int ID = DemoDeviceID;
    ModelDEMO();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};
