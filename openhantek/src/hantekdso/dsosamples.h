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
    int triggerPosition = -1;              ///< position for a triggered trace, < 0 = not triggered
    mutable QReadWriteLock lock;
};
