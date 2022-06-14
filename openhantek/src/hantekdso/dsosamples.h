// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "utils/printutils.h"
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <vector>

struct DSOsamples {
    std::vector< std::vector< double > > data; ///< Pointer to input data from device
    double samplerate = 0.0;                   ///< The samplerate of the input data
    unsigned char clipped = 0;                 ///< Bitmask of clipped channels
    bool liveTrigger = false;                  ///< live samples are triggered
    int triggeredPosition = 0;                 ///< position for a triggered trace, 0 = not triggered
    double pulseWidth1 = 0.0;                  ///< width from trigger point to next opposite slope
    double pulseWidth2 = 0.0;                  ///< width from next opposite slope to third slope
    Unit mathVoltageUnit = UNIT_VOLTS;         ///< unless UNIT_VOLTSQUARE for some math functions
    bool freeRunning = false;                  ///< trigger: NONE, half sample count
    unsigned tag = 0;                          ///< track individual sample blocks (debug support)
    mutable QReadWriteLock lock;
};

const int SAMPLESIZE = 20000;
const int SAMPLESIZE_ROLL = 40 * 256;
