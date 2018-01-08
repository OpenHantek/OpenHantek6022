#include "controlbegin.h"
#include "hantekprotocol/controlcode.h"

ControlBeginCommand::ControlBeginCommand(BulkIndex index) : ControlCommand(Hantek::ControlCode::CONTROL_BEGINCOMMAND, 10) {
    array[0] = 0x0f;
    array[1] = (uint8_t)index;
}
