// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ppresult.h"

class Processor {
  public:
    virtual ~Processor();
    virtual void process( PPresult * ) = 0;
};
