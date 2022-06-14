// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class HantekDsoControl;
using namespace Hantek;


struct ModelDSO6022BE : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO6022BE();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};


struct ModelDSO6022BL : public DSOModel {
    static const int ID = 0x602a;
    ModelDSO6022BL();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};


// Voltcraft DSO-2020 USB Oscilloscope
struct ModelDSO2020 : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO2020();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};


// two test cases with simple EZUSB board (LCsoft) without EEPROM or with Saleae VID/PID in EEPROM
// after loading the FW they look like a 6022BE (without useful sample values as Port B and D are left open)
// LCSOFT_TEST_BOARD is also used in modelDSO6022.cpp

// #define LCSOFT_TEST_BOARD

#ifdef LCSOFT_TEST_BOARD

struct ModelEzUSB : public DSOModel {
    static const int ID = 0x6022;
    ModelEzUSB();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};

struct ModelSaleae : public DSOModel {
    static const int ID = 0x6022;
    ModelSaleae();
    void applyRequirements( HantekDsoControl *dsoControl ) const override;
};

#endif
