#include "controlgetspeed.h"

ControlGetSpeed::ControlGetSpeed() : DataArray<uint8_t>(10) {}

ConnectionSpeed ControlGetSpeed::getSpeed() { return (ConnectionSpeed)this->array[0]; }

