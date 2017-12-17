#include "controlbegin.h"

//////////////////////////////////////////////////////////////////////////////
// class ControlBeginCommand
/// \brief Sets the command index to the given value.
/// \param index The CommandIndex for the command.
ControlBeginCommand::ControlBeginCommand(Hantek::BulkIndex index) : DataArray<uint8_t>(10) {
    this->init();

    this->setIndex(index);
}

/// \brief Gets the command index.
/// \return The CommandIndex for the command.
Hantek::BulkIndex ControlBeginCommand::getIndex() { return (Hantek::BulkIndex)this->array[1]; }

/// \brief Sets the command index to the given value.
/// \param index The new CommandIndex for the command.
void ControlBeginCommand::setIndex(Hantek::BulkIndex index) { memset(&(this->array[1]), (uint8_t)index, 3); }

/// \brief Initialize the array to the needed values.
void ControlBeginCommand::init() { this->array[0] = 0x0f; }
