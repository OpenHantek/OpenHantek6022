#include "controlgetspeed.h"
#include "hantekprotocol/controlcode.h"

ControlGetSpeed::ControlGetSpeed() : ControlCommand(Hantek::ControlCode::CONTROL_GETSPEED, 10) {}

ConnectionSpeed ControlGetSpeed::getSpeed() { return (ConnectionSpeed)this->array[0]; }

