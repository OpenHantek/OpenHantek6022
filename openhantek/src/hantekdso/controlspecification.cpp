// SPDX-License-Identifier: GPL-2.0+

#include "controlspecification.h"

Dso::ControlSpecification::ControlSpecification(unsigned channels) : channels(channels) {
    voltageLimit.resize(channels);
    voltageOffset.resize(channels);
}
