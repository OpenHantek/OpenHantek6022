
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "utils/dataarray.h"
#include <inttypes.h>
#include "usbdevicedefinitions.h"

/// \class ControlBeginCommand
/// \brief The CONTROL_BEGINCOMMAND builder.
class ControlBeginCommand : public DataArray<uint8_t> {
  public:
    /// \brief Sets the command index to the given value.
    /// \param index The CommandIndex for the command.
    ControlBeginCommand(BulkIndex index = COMMANDINDEX_0);
};
