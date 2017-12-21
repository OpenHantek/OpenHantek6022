#pragma once

#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"
#include "controlindexes.h"

using namespace Hantek;

struct ModelDSO6022BE : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO6022BE();
    virtual void applyRequirements(HantekDsoControl* dsoControl) const override;
};

struct ModelDSO6022LE : public ModelDSO6022BE {
    static const int ID = 0x6022;
    ModelDSO6022LE();
};
