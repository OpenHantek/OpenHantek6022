// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controlcode.h"
#include "controlcommand.h"
#include "types.h"
#include "usb/usbdevicedefinitions.h"

#include <inttypes.h>
#include <memory>

namespace Hantek {
struct CalibrationValues;


struct ControlSetGain_CH1 : public ControlCommand {
    ControlSetGain_CH1();
    void setGainCH1( uint8_t gain, uint8_t index );
};

struct ControlSetGain_CH2 : public ControlCommand {
    ControlSetGain_CH2();
    void setGainCH2( uint8_t gain, uint8_t index );
};

struct ControlSetSamplerate : public ControlCommand {
    ControlSetSamplerate();
    void setSamplerate( uint8_t sampleId, uint8_t index );
};

struct ControlSetNumChannels : public ControlCommand {
    ControlSetNumChannels();
    void setNumChannels( uint8_t val );
};

struct ControlStartSampling : public ControlCommand {
    ControlStartSampling();
};

struct ControlStopSampling : public ControlCommand {
    ControlStopSampling();
};

struct ControlGetCalibration : public ControlCommand {
    ControlGetCalibration();
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

extern const std::vector< QString > controlNames;

} // namespace Hantek
