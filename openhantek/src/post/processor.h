#pragma once

#include "ppresult.h"

class Processor {
public:
    virtual void process(PPresult*) = 0;
};
