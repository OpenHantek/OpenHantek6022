
// SPDX-License-Identifier: GPL-2.0+

#include "dsomodel.h"

DSOModel::DSOModel(int id, long vendorID, long productID, long vendorIDnoFirmware,
                   long productIDnoFirmware, const std::string &firmwareToken, const std::string &name, const Hantek::ControlSpecification &specification)
    : ID(id), vendorID(vendorID), productID(productID), vendorIDnoFirmware(vendorIDnoFirmware),
      productIDnoFirmware(productIDnoFirmware), firmwareToken(firmwareToken), name(name), specification(specification) {}
