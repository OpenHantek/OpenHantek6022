#include "controlbegin.h"

ControlBeginCommand::ControlBeginCommand(BulkIndex index) : DataArray<uint8_t>(10) {
    array[0] = 0x0f;
    array[1] = (uint8_t)index;
}
