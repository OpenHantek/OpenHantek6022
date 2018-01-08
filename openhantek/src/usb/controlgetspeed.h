// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "controlcommand.h"
#include "usbdevicedefinitions.h"

/// \brief The CONTROL_GETSPEED parser.
class ControlGetSpeed : public ControlCommand {
  public:
    ControlGetSpeed();
    /// \brief Gets the speed of the connection.
    /// \return The speed level of the USB connection.
    ConnectionSpeed getSpeed();
};
