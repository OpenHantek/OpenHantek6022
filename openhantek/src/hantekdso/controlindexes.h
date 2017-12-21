// SPDX-License-Identifier: GPL-2.0+

#pragma once

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \enum ControlIndex
/// \brief The array indices for the waiting control commands.
enum ControlIndex {
    // CONTROLINDEX_VALUE,
    // CONTROLINDEX_GETSPEED,
    // CONTROLINDEX_BEGINCOMMAND,
    CONTROLINDEX_SETOFFSET,
    CONTROLINDEX_SETRELAYS,
    CONTROLINDEX_SETVOLTDIV_CH1,
    CONTROLINDEX_SETVOLTDIV_CH2,
    CONTROLINDEX_SETTIMEDIV,
    CONTROLINDEX_ACQUIIRE_HARD_DATA,
    CONTROLINDEX_COUNT
};

}
