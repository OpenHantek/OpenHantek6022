#pragma once

#include "definitions.h"
#include "utils/dataarray.h"

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \class ControlGetSpeed                                      hantek/types.h
/// \brief The CONTROL_GETSPEED parser.
class ControlGetSpeed : public DataArray<uint8_t> {
public:
    ControlGetSpeed();

    ConnectionSpeed getSpeed();
};


//////////////////////////////////////////////////////////////////////////////
/// \class ControlSetOffset                                     hantek/types.h
/// \brief The CONTROL_SETOFFSET builder.
class ControlSetOffset : public DataArray<uint8_t> {
public:
    ControlSetOffset();
    ControlSetOffset(uint16_t channel1, uint16_t channel2, uint16_t trigger);

    uint16_t getChannel(unsigned int channel);
    void setChannel(unsigned int channel, uint16_t offset);
    uint16_t getTrigger();
    void setTrigger(uint16_t level);

private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class ControlSetRelays                                     hantek/types.h
/// \brief The CONTROL_SETRELAYS builder.
class ControlSetRelays : public DataArray<uint8_t> {
public:
    ControlSetRelays(bool ch1Below1V = false, bool ch1Below100mV = false,
                     bool ch1CouplingDC = false, bool ch2Below1V = false,
                     bool ch2Below100mV = false, bool ch2CouplingDC = false,
                     bool triggerExt = false);

    bool getBelow1V(unsigned int channel);
    void setBelow1V(unsigned int channel, bool below);
    bool getBelow100mV(unsigned int channel);
    void setBelow100mV(unsigned int channel, bool below);
    bool getCoupling(unsigned int channel);
    void setCoupling(unsigned int channel, bool dc);
    bool getTrigger();
    void setTrigger(bool ext);
};

//////////////////////////////////////////////////////////////////////////////
/// \class ControlSetVoltDIV_CH1 hantek/types.h
/// \brief The CONTROL_SETVOLTDIV_CH1 builder.
class ControlSetVoltDIV_CH1 : public DataArray<uint8_t> {
public:
    ControlSetVoltDIV_CH1();
    void setDiv(uint8_t val);
};

//////////////////////////////////////////////////////////////////////////////
/// \class ControlSetVoltDIV_CH2 hantek/types.h
/// \brief The CONTROL_SETVOLTDIV_CH2 builder.
class ControlSetVoltDIV_CH2 : public DataArray<uint8_t> {
public:
    ControlSetVoltDIV_CH2();
    void setDiv(uint8_t val);
};

//////////////////////////////////////////////////////////////////////////////
/// \class ControlSetTimeDIV hantek/types.h
/// \brief The CONTROL_SETTIMEDIV builder.
class ControlSetTimeDIV : public DataArray<uint8_t> {
public:
    ControlSetTimeDIV();
    void setDiv(uint8_t val);
};

//////////////////////////////////////////////////////////////////////////////
/// \class ControlAcquireHardData hantek/types.h
/// \brief The CONTROL_ACQUIIRE_HARD_DATA builder.
class ControlAcquireHardData : public DataArray<uint8_t> {
public:
    ControlAcquireHardData();

private:
    void init();
};

}

