// SPDX-License-Identifier: GPL-2.0+

#pragma once

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \enum RollState
/// \brief The states of the roll cycle (Since capture state isn't valid).
enum RollState {
    ROLL_STARTSAMPLING = 0, ///< Start sampling
    ROLL_ENABLETRIGGER = 1, ///< Enable triggering
    ROLL_FORCETRIGGER = 2,  ///< Force triggering
    ROLL_GETDATA = 3,       ///< Request sample data
    ROLL_COUNT
};

}
