// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <vector>

struct DSOsamples {
    std::vector<std::vector<double>> data; ///< Pointer to input data from device
    double samplerate = 0.0;               ///< The samplerate of the input data
    bool append = false;                   ///< true, if waiting data should be appended
    mutable QReadWriteLock lock;
};
