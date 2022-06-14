// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controlspecification.h"
#include <QString>
#include <list>
#include <string>

class HantekDsoControl;

/**
 * @brief Describes a device
 * This is the central class to describe a hantek compatible DSO. It contains all usb information to find
 * the device on the bus, references to the firmware as well as the user visible name and device specification.
 */
class DSOModel {
  public:
    const int ID;
    const unsigned vendorID;            ///< The USB vendor ID
    const unsigned productID;           ///< The USB product ID
    const unsigned vendorIDnoFirmware;  ///< The USB vendor ID if no firmware is uploaded yet
    const unsigned productIDnoFirmware; ///< The USB product ID if no firmware is uploaded yet
    const unsigned firmwareVersion;     ///< The BCD version of the uploaded firmware
    /// Firmwares are compiled into the executable with a filename pattern of devicename-firmware.hex and
    /// devicename-loader.hex.
    /// The firmwareToken is the "devicename" of the pattern above.
    const QString firmwareToken;
    const QString name; ///< User visible name. Does not need internationalisation/translation.

  protected:
    Dso::ControlSpecification specification;

  public:
    /// This model may need to modify the HantekDsoControl class to work correctly
    virtual void applyRequirements( HantekDsoControl * ) const = 0;
    DSOModel( int id, unsigned vendorID, unsigned productID, unsigned vendorIDnoFirmware, unsigned productIDnoFirmware,
              unsigned firmwareVersion, const QString &firmwareToken, const QString &name,
              const Dso::ControlSpecification &&specification );
    virtual ~DSOModel() = default;
    /// Return the device specifications
    inline const Dso::ControlSpecification *spec() const { return &specification; }
};
