// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

struct DSOsamples {
    std::vector<std::vector<double>> data; ///< Pointer to input data from device
    double samplerate = 0.0; ///< The samplerate of the input data
    bool append = false;     ///< true, if waiting data should be appended
    mutable QReadWriteLock lock;
};
