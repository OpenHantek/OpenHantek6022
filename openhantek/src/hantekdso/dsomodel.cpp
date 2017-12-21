
// SPDX-License-Identifier: GPL-2.0+

#include "dsomodel.h"

DSOModel::DSOModel(int id, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware, std::__cxx11::string firmwareToken, const std::__cxx11::string name, const Hantek::ControlSpecification &specification)
    : ID(id), vendorID(vendorID), productID(productID), vendorIDnoFirmware(vendorIDnoFirmware),
      productIDnoFirmware(productIDnoFirmware), firmwareToken(firmwareToken), name(name), specification(specification) {}
