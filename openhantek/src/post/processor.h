// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ppresult.h"

class Processor {
public:
    virtual void process(PPresult*) = 0;
};
