#include "controlspecification.h"

Dso::ControlSpecification::ControlSpecification(unsigned channels) : channels(channels), cmdGetLimits(channels)
{
    voltageLimit.resize(channels);
}
