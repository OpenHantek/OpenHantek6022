// SPDX-License-Identifier: GPL-2.0+

#include <cstring>

#include "controlStructs.h"
#include "controlvalue.h"
#include "definitions.h"

namespace Hantek {

ControlBeginCommand::ControlBeginCommand(BulkIndex index)
    : ControlCommand(Hantek::ControlCode::CONTROL_BEGINCOMMAND, 10) {
    data()[0] = 0x0f;
    data()[1] = (uint8_t)index;
}

ControlGetSpeed::ControlGetSpeed() : ControlCommand(Hantek::ControlCode::CONTROL_GETSPEED, 10) {}

ConnectionSpeed ControlGetSpeed::getSpeed() { return (ConnectionSpeed)data()[0]; }

ControlSetOffset::ControlSetOffset() : ControlCommand(ControlCode::CONTROL_SETOFFSET, 17) {}

ControlSetOffset::ControlSetOffset(uint16_t channel1, uint16_t channel2, uint16_t trigger)
    : ControlCommand(ControlCode::CONTROL_SETOFFSET, 17) {
    this->setChannel(0, channel1);
    this->setChannel(1, channel2);
    this->setTrigger(trigger);
}

uint16_t ControlSetOffset::getChannel(ChannelID channel) {
    if (channel == 0)
        return ((data()[0] & 0x0f) << 8) | data()[1];
    else
        return ((data()[2] & 0x0f) << 8) | data()[3];
}

void ControlSetOffset::setChannel(ChannelID channel, uint16_t offset) {
    if (channel == 0) {
        data()[0] = (uint8_t)(offset >> 8);
        data()[1] = (uint8_t)offset;
    } else {
        data()[2] = (uint8_t)(offset >> 8);
        data()[3] = (uint8_t)offset;
    }
}

uint16_t ControlSetOffset::getTrigger() { return ((data()[4] & 0x0f) << 8) | data()[5]; }

void ControlSetOffset::setTrigger(uint16_t level) {
    data()[4] = (uint8_t)(level >> 8);
    data()[5] = (uint8_t)level;
}

ControlSetRelays::ControlSetRelays(bool ch1Below1V, bool ch1Below100mV, bool ch1CouplingDC, bool ch2Below1V,
                                   bool ch2Below100mV, bool ch2CouplingDC, bool triggerExt)
    : ControlCommand(ControlCode::CONTROL_SETRELAYS, 17) {
    this->setBelow1V(0, ch1Below1V);
    this->setBelow100mV(0, ch1Below100mV);
    this->setCoupling(0, ch1CouplingDC);
    this->setBelow1V(1, ch2Below1V);
    this->setBelow100mV(1, ch2Below100mV);
    this->setCoupling(1, ch2CouplingDC);
    this->setTrigger(triggerExt);
}

bool ControlSetRelays::getBelow1V(ChannelID channel) {
    if (channel == 0)
        return (data()[1] & 0x04) == 0x00;
    else
        return (data()[4] & 0x20) == 0x00;
}

void ControlSetRelays::setBelow1V(ChannelID channel, bool below) {
    if (channel == 0)
        data()[1] = below ? 0xfb : 0x04;
    else
        data()[4] = below ? 0xdf : 0x20;
}

bool ControlSetRelays::getBelow100mV(ChannelID channel) {
    if (channel == 0)
        return (data()[2] & 0x08) == 0x00;
    else
        return (data()[5] & 0x40) == 0x00;
}

void ControlSetRelays::setBelow100mV(ChannelID channel, bool below) {
    if (channel == 0)
        data()[2] = below ? 0xf7 : 0x08;
    else
        data()[5] = below ? 0xbf : 0x40;
}

bool ControlSetRelays::getCoupling(ChannelID channel) {
    if (channel == 0)
        return (data()[3] & 0x02) == 0x00;
    else
        return (data()[6] & 0x10) == 0x00;
}

void ControlSetRelays::setCoupling(ChannelID channel, bool dc) {
    if (channel == 0)
        data()[3] = dc ? 0xfd : 0x02;
    else
        data()[6] = dc ? 0xef : 0x10;
}

bool ControlSetRelays::getTrigger() { return (data()[7] & 0x01) == 0x00; }

void ControlSetRelays::setTrigger(bool ext) { data()[7] = ext ? 0xfe : 0x01; }

ControlSetVoltDIV_CH1::ControlSetVoltDIV_CH1() : ControlCommand(ControlCode::CONTROL_SETVOLTDIV_CH1, 1) {
    this->setDiv(5);
}

void ControlSetVoltDIV_CH1::setDiv(uint8_t val) { data()[0] = val; }

ControlSetVoltDIV_CH2::ControlSetVoltDIV_CH2() : ControlCommand(ControlCode::CONTROL_SETVOLTDIV_CH2, 1) {
    this->setDiv(5);
}

void ControlSetVoltDIV_CH2::setDiv(uint8_t val) { data()[0] = val; }

ControlSetTimeDIV::ControlSetTimeDIV() : ControlCommand(ControlCode::CONTROL_SETTIMEDIV, 1) { this->setDiv(1); }

void ControlSetTimeDIV::setDiv(uint8_t val) { data()[0] = val; }

ControlAcquireHardData::ControlAcquireHardData() : ControlCommand(ControlCode::CONTROL_ACQUIIRE_HARD_DATA, 1) {
    data()[0] = 0x01;
}

ControlGetLimits::ControlGetLimits(size_t channels)
    : ControlCommand(ControlCode::CONTROL_VALUE, 1), offsetLimit(new OffsetsPerGainStep[channels]) {
    value = (uint8_t)ControlValue::VALUE_OFFSETLIMITS;
    data()[0] = 0x01;
}
}
