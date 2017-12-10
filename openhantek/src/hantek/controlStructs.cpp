// SPDX-License-Identifier: GPL-2.0+

#include <cstring>

#include "controlStructs.h"

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
// class ControlGetSpeed
/// \brief Initializes the array.
ControlGetSpeed::ControlGetSpeed() : DataArray<uint8_t>(10) {}

/// \brief Gets the speed of the connection.
/// \return The speed level of the USB connection.
ConnectionSpeed ControlGetSpeed::getSpeed() {
  return (ConnectionSpeed)this->array[0];
}

//////////////////////////////////////////////////////////////////////////////
// class ControlSetOffset
/// \brief Sets the data array to the default values.
ControlSetOffset::ControlSetOffset() : DataArray<uint8_t>(17) {}

/// \brief Sets the offsets to the given values.
/// \param channel1 The offset for channel 1.
/// \param channel2 The offset for channel 2.
/// \param trigger The offset for ext. trigger.
ControlSetOffset::ControlSetOffset(uint16_t channel1, uint16_t channel2,
                                   uint16_t trigger)
    : DataArray<uint8_t>(17) {
  this->setChannel(0, channel1);
  this->setChannel(1, channel2);
  this->setTrigger(trigger);
}

/// \brief Get the offset for the given channel.
/// \param channel The channel whose offset should be returned.
/// \return The channel offset value.
uint16_t ControlSetOffset::getChannel(unsigned int channel) {
  if (channel == 0)
    return ((this->array[0] & 0x0f) << 8) | this->array[1];
  else
    return ((this->array[2] & 0x0f) << 8) | this->array[3];
}

/// \brief Set the offset for the given channel.
/// \param channel The channel that should be set.
/// \param offset The new channel offset value.
void ControlSetOffset::setChannel(unsigned int channel, uint16_t offset) {
  if (channel == 0) {
    this->array[0] = (uint8_t)(offset >> 8);
    this->array[1] = (uint8_t)offset;
  } else {
    this->array[2] = (uint8_t)(offset >> 8);
    this->array[3] = (uint8_t)offset;
  }
}

/// \brief Get the trigger level.
/// \return The trigger level value.
uint16_t ControlSetOffset::getTrigger() {
  return ((this->array[4] & 0x0f) << 8) | this->array[5];
}

/// \brief Set the trigger level.
/// \param level The new trigger level value.
void ControlSetOffset::setTrigger(uint16_t level) {
  this->array[4] = (uint8_t)(level >> 8);
  this->array[5] = (uint8_t)level;
}

//////////////////////////////////////////////////////////////////////////////
// class ControlSetRelays
/// \brief Sets all relay states.
/// \param ch1Below1V Sets the state of the Channel 1 below 1 V relay.
/// \param ch1Below100mV Sets the state of the Channel 1 below 100 mV relay.
/// \param ch1CouplingDC Sets the state of the Channel 1 coupling relay.
/// \param ch2Below1V Sets the state of the Channel 2 below 1 V relay.
/// \param ch2Below100mV Sets the state of the Channel 2 below 100 mV relay.
/// \param ch2CouplingDC Sets the state of the Channel 2 coupling relay.
/// \param triggerExt Sets the state of the external trigger relay.
ControlSetRelays::ControlSetRelays(bool ch1Below1V, bool ch1Below100mV,
                                   bool ch1CouplingDC, bool ch2Below1V,
                                   bool ch2Below100mV, bool ch2CouplingDC,
                                   bool triggerExt)
    : DataArray<uint8_t>(17) {
  this->setBelow1V(0, ch1Below1V);
  this->setBelow100mV(0, ch1Below100mV);
  this->setCoupling(0, ch1CouplingDC);
  this->setBelow1V(1, ch2Below1V);
  this->setBelow100mV(1, ch2Below100mV);
  this->setCoupling(1, ch2CouplingDC);
  this->setTrigger(triggerExt);
}

/// \brief Get the below 1 V relay state for the given channel.
/// \param channel The channel whose relay state should be returned.
/// \return true, if the gain of the channel is below 1 V.
bool ControlSetRelays::getBelow1V(unsigned int channel) {
  if (channel == 0)
    return (this->array[1] & 0x04) == 0x00;
  else
    return (this->array[4] & 0x20) == 0x00;
}

/// \brief Set the below 1 V relay for the given channel.
/// \param channel The channel that should be set.
/// \param below true, if the gain of the channel should be below 1 V.
void ControlSetRelays::setBelow1V(unsigned int channel, bool below) {
  if (channel == 0)
    this->array[1] = below ? 0xfb : 0x04;
  else
    this->array[4] = below ? 0xdf : 0x20;
}

/// \brief Get the below 1 V relay state for the given channel.
/// \param channel The channel whose relay state should be returned.
/// \return true, if the gain of the channel is below 1 V.
bool ControlSetRelays::getBelow100mV(unsigned int channel) {
  if (channel == 0)
    return (this->array[2] & 0x08) == 0x00;
  else
    return (this->array[5] & 0x40) == 0x00;
}

/// \brief Set the below 100 mV relay for the given channel.
/// \param channel The channel that should be set.
/// \param below true, if the gain of the channel should be below 100 mV.
void ControlSetRelays::setBelow100mV(unsigned int channel, bool below) {
  if (channel == 0)
    this->array[2] = below ? 0xf7 : 0x08;
  else
    this->array[5] = below ? 0xbf : 0x40;
}

/// \brief Get the coupling relay state for the given channel.
/// \param channel The channel whose relay state should be returned.
/// \return true, if the coupling of the channel is DC.
bool ControlSetRelays::getCoupling(unsigned int channel) {
  if (channel == 0)
    return (this->array[3] & 0x02) == 0x00;
  else
    return (this->array[6] & 0x10) == 0x00;
}

/// \brief Set the coupling relay for the given channel.
/// \param channel The channel that should be set.
/// \param dc true, if the coupling of the channel should be DC.
void ControlSetRelays::setCoupling(unsigned int channel, bool dc) {
  if (channel == 0)
    this->array[3] = dc ? 0xfd : 0x02;
  else
    this->array[6] = dc ? 0xef : 0x10;
}

/// \brief Get the external trigger relay state.
/// \return true, if the trigger is external (EXT-Connector).
bool ControlSetRelays::getTrigger() { return (this->array[7] & 0x01) == 0x00; }

/// \brief Set the external trigger relay.
/// \param ext true, if the trigger should be external (EXT-Connector).
void ControlSetRelays::setTrigger(bool ext) {
  this->array[7] = ext ? 0xfe : 0x01;
}

//////////////////////////////////////////////////////////////////////////////
// class ControlSetVoltDIV_CH1
/// \brief Sets the data array to the default values.
ControlSetVoltDIV_CH1::ControlSetVoltDIV_CH1() : DataArray<uint8_t>(1) {
  this->setDiv(5);
}

void ControlSetVoltDIV_CH1::setDiv(uint8_t val) { this->array[0] = val; }

//////////////////////////////////////////////////////////////////////////////
// class ControlSetVoltDIV_CH2
/// \brief Sets the data array to the default values.
ControlSetVoltDIV_CH2::ControlSetVoltDIV_CH2() : DataArray<uint8_t>(1) {
  this->setDiv(5);
}

void ControlSetVoltDIV_CH2::setDiv(uint8_t val) { this->array[0] = val; }

//////////////////////////////////////////////////////////////////////////////
// class ControlSetTimeDIV
/// \brief Sets the data array to the default values.
ControlSetTimeDIV::ControlSetTimeDIV() : DataArray<uint8_t>(1) {
  this->setDiv(1);
}

void ControlSetTimeDIV::setDiv(uint8_t val) { this->array[0] = val; }

//////////////////////////////////////////////////////////////////////////////
// class ControlAcquireHardData
/// \brief Sets the data array to the default values.
ControlAcquireHardData::ControlAcquireHardData()
    : DataArray<uint8_t>(1) {
  this->init();
}

void ControlAcquireHardData::init() { this->array[0] = 0x01; }
}
