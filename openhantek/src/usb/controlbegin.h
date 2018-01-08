
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "controlcommand.h"
#include <inttypes.h>

/// \enum BulkIndex
/// \brief Can be set by CONTROL_BEGINCOMMAND, maybe it allows multiple commands
/// at the same time?
enum BulkIndex {
    COMMANDINDEX_0 = 0x03, ///< Used most of the time
    COMMANDINDEX_1 = 0x0a,
    COMMANDINDEX_2 = 0x09,
    COMMANDINDEX_3 = 0x01, ///< Used for ::BulkCode::SETTRIGGERANDSAMPLERATE sometimes
    COMMANDINDEX_4 = 0x02,
    COMMANDINDEX_5 = 0x08
};

/// \class ControlBeginCommand
class ControlBeginCommand : public ControlCommand {
  public:
    /// \brief Sets the command index to the given value.
    /// \param index The CommandIndex for the command.
    ControlBeginCommand(BulkIndex index = COMMANDINDEX_0);
};
