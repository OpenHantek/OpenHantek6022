// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <vector>

struct DSOsamples {
    std::vector<std::vector<double>> data; ///< Pointer to input data from device
    double samplerate = 0.0;               ///< The samplerate of the input data
    unsigned char clipped = 0;             ///< Bitmask of clipped channels
    bool liveTrigger = false;              ///< live samples are triggered
    unsigned triggerPosition = 0;          ///< position for a triggered trace, 0 = not triggered
    double pulseWidth1 = 0.0;              ///< width from trigger point to next opposite slope
    double pulseWidth2 = 0.0;              ///< width from next opposite slope to third slope
    mutable QReadWriteLock lock;
};
