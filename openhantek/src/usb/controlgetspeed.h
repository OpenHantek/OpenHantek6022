
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "utils/dataarray.h"
#include <inttypes.h>
#include "usbdevicedefinitions.h"

/// \class ControlGetSpeed
/// \brief The CONTROL_GETSPEED parser.
class ControlGetSpeed : public DataArray<uint8_t> {
  public:
    ControlGetSpeed();
    /// \brief Gets the speed of the connection.
    /// \return The speed level of the USB connection.
    ConnectionSpeed getSpeed();
};
