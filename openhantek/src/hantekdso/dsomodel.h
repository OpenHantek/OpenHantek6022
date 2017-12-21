
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <list>
#include <string>
#include "controlspecification.h"

class HantekDsoControl;

/**
 * @brief Describes a device
 * This is the central class to describe a hantek compatible DSO. It contains all usb information to find
 * the device on the bus, references to the firmware as well as the user visible name and device specification.
 */
class DSOModel {
  public:
    int ID;
    long vendorID; ///< The USB vendor ID
    long productID; ///< The USB product ID
    long vendorIDnoFirmware; ///< The USB vendor ID if no firmware is flashed yet
    long productIDnoFirmware; ///< The USB product ID if no firmware is flashed yet
    /// Firmwares are compiled into the executable with a filename pattern of devicename-firmware.hex and devicename-loader.hex.
    /// The firmwareToken is the "devicename" of the pattern above.
    std::string firmwareToken;
    std::string name; ///< User visible name. Does not need internationalisation/translation.
    Hantek::ControlSpecification specification;
  public:
    /// This model may need to modify the HantekDsoControl class to work correctly
    virtual void applyRequirements(HantekDsoControl*) const = 0;
    DSOModel(int id, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware,
             std::string firmwareToken, const std::string name, const Hantek::ControlSpecification& specification);
    virtual ~DSOModel() = default;
};

