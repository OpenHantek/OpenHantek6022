// SPDX-License-Identifier: GPL-2.0+

#include <cstring>

#include "controlStructs.h"

namespace Hantek {

ControlSetOffset::ControlSetOffset() : DataArray<uint8_t>(17) {}

ControlSetOffset::ControlSetOffset(uint16_t channel1, uint16_t channel2, uint16_t trigger) : DataArray<uint8_t>(17) {
    this->setChannel(0, channel1);
    this->setChannel(1, channel2);
    this->setTrigger(trigger);
}

uint16_t ControlSetOffset::getChannel(unsigned int channel) {
    if (channel == 0)
        return ((this->array[0] & 0x0f) << 8) | this->array[1];
    else
        return ((this->array[2] & 0x0f) << 8) | this->array[3];
}

void ControlSetOffset::setChannel(unsigned int channel, uint16_t offset) {
    if (channel == 0) {
        this->array[0] = (uint8_t)(offset >> 8);
        this->array[1] = (uint8_t)offset;
    } else {
        this->array[2] = (uint8_t)(offset >> 8);
        this->array[3] = (uint8_t)offset;
    }
}

uint16_t ControlSetOffset::getTrigger() { return ((this->array[4] & 0x0f) << 8) | this->array[5]; }

void ControlSetOffset::setTrigger(uint16_t level) {
    this->array[4] = (uint8_t)(level >> 8);
    this->array[5] = (uint8_t)level;
}

ControlSetRelays::ControlSetRelays(bool ch1Below1V, bool ch1Below100mV, bool ch1CouplingDC, bool ch2Below1V,
                                   bool ch2Below100mV, bool ch2CouplingDC, bool triggerExt)
    : DataArray<uint8_t>(17) {
    this->setBelow1V(0, ch1Below1V);
    this->setBelow100mV(0, ch1Below100mV);
    this->setCoupling(0, ch1CouplingDC);
    this->setBelow1V(1, ch2Below1V);
    this->setBelow100mV(1, ch2Below100mV);
    this->setCoupling(1, ch2CouplingDC);
    this->setTrigger(triggerExt);
}

bool ControlSetRelays::getBelow1V(unsigned int channel) {
    if (channel == 0)
        return (this->array[1] & 0x04) == 0x00;
    else
        return (this->array[4] & 0x20) == 0x00;
}

void ControlSetRelays::setBelow1V(unsigned int channel, bool below) {
    if (channel == 0)
        this->array[1] = below ? 0xfb : 0x04;
    else
        this->array[4] = below ? 0xdf : 0x20;
}

bool ControlSetRelays::getBelow100mV(unsigned int channel) {
    if (channel == 0)
        return (this->array[2] & 0x08) == 0x00;
    else
        return (this->array[5] & 0x40) == 0x00;
}

void ControlSetRelays::setBelow100mV(unsigned int channel, bool below) {
    if (channel == 0)
        this->array[2] = below ? 0xf7 : 0x08;
    else
        this->array[5] = below ? 0xbf : 0x40;
}

bool ControlSetRelays::getCoupling(unsigned int channel) {
    if (channel == 0)
        return (this->array[3] & 0x02) == 0x00;
    else
        return (this->array[6] & 0x10) == 0x00;
}

void ControlSetRelays::setCoupling(unsigned int channel, bool dc) {
    if (channel == 0)
        this->array[3] = dc ? 0xfd : 0x02;
    else
        this->array[6] = dc ? 0xef : 0x10;
}

bool ControlSetRelays::getTrigger() { return (this->array[7] & 0x01) == 0x00; }

void ControlSetRelays::setTrigger(bool ext) { this->array[7] = ext ? 0xfe : 0x01; }

ControlSetVoltDIV_CH1::ControlSetVoltDIV_CH1() : DataArray<uint8_t>(1) { this->setDiv(5); }

void ControlSetVoltDIV_CH1::setDiv(uint8_t val) { this->array[0] = val; }

ControlSetVoltDIV_CH2::ControlSetVoltDIV_CH2() : DataArray<uint8_t>(1) { this->setDiv(5); }

void ControlSetVoltDIV_CH2::setDiv(uint8_t val) { this->array[0] = val; }

ControlSetTimeDIV::ControlSetTimeDIV() : DataArray<uint8_t>(1) { this->setDiv(1); }

void ControlSetTimeDIV::setDiv(uint8_t val) { this->array[0] = val; }

ControlAcquireHardData::ControlAcquireHardData() : DataArray<uint8_t>(1) { this->array[0] = 0x01; }
}
