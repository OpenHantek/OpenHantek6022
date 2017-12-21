// SPDX-License-Identifier: GPL-2.0+

#pragma once

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \enum RollState
/// \brief The states of the roll cycle (Since capture state isn't valid).
enum class RollState : int {
    STARTSAMPLING = 0, ///< Start sampling
    ENABLETRIGGER = 1, ///< Enable triggering
    FORCETRIGGER = 2,  ///< Force triggering
    GETDATA = 3,       ///< Request sample data

    _COUNT // Used for mod operator
};

//////////////////////////////////////////////////////////////////////////////
/// \enum CaptureState                                          hantek/types.h
/// \brief The different capture states which the oscilloscope returns.
enum CaptureState {
    CAPTURE_WAITING = 0,   ///< The scope is waiting for a trigger event
    CAPTURE_SAMPLING = 1,  ///< The scope is sampling data after triggering
    CAPTURE_READY = 2,     ///< Sampling data is available (DSO-2090/DSO-2150)
    CAPTURE_READY2250 = 3, ///< Sampling data is available (DSO-2250)
    CAPTURE_READY5200 = 7, ///< Sampling data is available (DSO-5200/DSO-5200A)
    CAPTURE_ERROR = 1000
};

}
