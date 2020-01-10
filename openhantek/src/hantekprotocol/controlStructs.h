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

/// \enum CommandIndex
/// \brief Can be set by CONTROL_BEGINCOMMAND, maybe it allows multiple commands
/// at the same time?
enum CommandIndex {
    COMMANDINDEX_0 = 0x03, ///< Used most of the time
    COMMANDINDEX_1 = 0x0a,
    COMMANDINDEX_2 = 0x09,
    COMMANDINDEX_3 = 0x01, ///< Used for ::BulkCode::SETTRIGGERANDSAMPLERATE sometimes
    COMMANDINDEX_4 = 0x02,
    COMMANDINDEX_5 = 0x08
};

/// \class ControlBeginCommand
class ControlBeginCommand : public ControlCommand {
  public:
    /// \brief Sets the command index to the given value.
    /// \param index The CommandIndex for the command.
    explicit ControlBeginCommand( CommandIndex index = COMMANDINDEX_0 );
};

/// \brief The CONTROL_GETSPEED parser.
class ControlGetSpeed : public ControlCommand {
  public:
    ControlGetSpeed();
    /// \brief Gets the speed of the connection.
    /// \return The speed level of the USB connection.
//     ConnectionSpeed getSpeed();
};

struct ControlSetVoltDIV_CH1 : public ControlCommand {
    ControlSetVoltDIV_CH1();
    void setDiv(uint8_t val);
};

struct ControlSetVoltDIV_CH2 : public ControlCommand {
    ControlSetVoltDIV_CH2();
    void setDiv(uint8_t val);
};

struct ControlSetTimeDIV : public ControlCommand {
    ControlSetTimeDIV();
    void setDiv(uint8_t val);
};

struct ControlSetNumChannels : public ControlCommand {
    ControlSetNumChannels();
    void setDiv(uint8_t val);
};

struct ControlAcquireHardData : public ControlCommand {
    ControlAcquireHardData();
};

struct ControlGetLimits : public ControlCommand {
    ControlGetLimits();
};

struct ControlSetCalFreq : public ControlCommand {
    ControlSetCalFreq();
    void setCalFreq(uint8_t val);
};

struct ControlSetCoupling : public ControlCommand {
    ControlSetCoupling();
    void setCoupling(ChannelID channel, bool dc);
    bool ch1Coupling, ch2Coupling;
};

}
