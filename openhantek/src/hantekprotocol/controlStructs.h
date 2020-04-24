// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "controlcode.h"
#include "controlcommand.h"
#include "types.h"
#include "usb/usbdevicedefinitions.h"

#include <inttypes.h>
#include <memory>

namespace Hantek {
struct CalibrationValues;

struct ControlSetVoltDIV_CH1 : public ControlCommand {
    ControlSetVoltDIV_CH1();
    void setDiv( uint8_t val );
};

struct ControlSetVoltDIV_CH2 : public ControlCommand {
    ControlSetVoltDIV_CH2();
    void setDiv( uint8_t val );
};

struct ControlSetTimeDIV : public ControlCommand {
    ControlSetTimeDIV();
    void setDiv( uint8_t val );
};

struct ControlSetNumChannels : public ControlCommand {
    ControlSetNumChannels();
    void setDiv( uint8_t val );
};

struct ControlStartSampling : public ControlCommand {
    ControlStartSampling();
};

struct ControlStopSampling : public ControlCommand {
    ControlStopSampling();
};

struct ControlGetLimits : public ControlCommand {
    ControlGetLimits();
};

struct ControlSetCalFreq : public ControlCommand {
    ControlSetCalFreq();
    void setCalFreq( uint8_t val );
};

struct ControlSetCoupling : public ControlCommand {
    ControlSetCoupling();
    void setCoupling( ChannelID channel, bool dc );
    uint8_t ch1Coupling, ch2Coupling;
};

} // namespace Hantek
