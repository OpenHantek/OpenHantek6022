#include "controlcommand.h"

ControlCommand::ControlCommand(Hantek::ControlCode code, unsigned size): DataArray<uint8_t>(size), code((uint8_t)code) {}
