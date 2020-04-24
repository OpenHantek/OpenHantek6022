// SPDX-License-Identifier: GPL-2.0+

#include <cstring>

#include "controlStructs.h"
#include "definitions.h"

namespace Hantek {

ControlSetVoltDIV_CH1::ControlSetVoltDIV_CH1() : ControlCommand( ControlCode::CONTROL_SETVOLTDIV_CH1, 1 ) { this->setDiv( 5 ); }

void ControlSetVoltDIV_CH1::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetVoltDIV_CH2::ControlSetVoltDIV_CH2() : ControlCommand( ControlCode::CONTROL_SETVOLTDIV_CH2, 1 ) { this->setDiv( 5 ); }

void ControlSetVoltDIV_CH2::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetTimeDIV::ControlSetTimeDIV() : ControlCommand( ControlCode::CONTROL_SETTIMEDIV, 1 ) { this->setDiv( 1 ); }

void ControlSetTimeDIV::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetNumChannels::ControlSetNumChannels() : ControlCommand( ControlCode::CONTROL_SETNUMCHANNELS, 1 ) { this->setDiv( 2 ); }

void ControlSetNumChannels::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlStartSampling::ControlStartSampling() : ControlCommand( ControlCode::CONTROL_STARTSAMPLING, 1 ) { data()[ 0 ] = 0x01; }


ControlStopSampling::ControlStopSampling() : ControlCommand( ControlCode::CONTROL_STARTSAMPLING, 1 ) { data()[ 0 ] = 0x00; }


ControlGetLimits::ControlGetLimits() : ControlCommand( ControlCode::CONTROL_GETEEPROM, sizeof( CalibrationValues ) ) {
    value = uint8_t( 8 ); // get calibration values from EEPROM offset 8
    data()[ 0 ] = 0x01;
}


ControlSetCalFreq::ControlSetCalFreq() : ControlCommand( ControlCode::CONTROL_SETCALFREQ, 1 ) {
    this->setCalFreq( 1 ); // 1kHz
}

void ControlSetCalFreq::setCalFreq( uint8_t val ) { data()[ 0 ] = val; }


ControlSetCoupling::ControlSetCoupling()
    : ControlCommand( ControlCode::CONTROL_SETCOUPLING, 1 ), ch1Coupling( 0x01 ), ch2Coupling( 0x10 ) {
    data()[ 0 ] = 0x11;
}

void ControlSetCoupling::setCoupling( ChannelID channel, bool dc ) {
    if ( channel == 0 )
        ch1Coupling = dc ? 0x01 : 0x00;
    else
        ch2Coupling = dc ? 0x10 : 0x00;
    data()[ 0 ] = 0xFF & ( ch2Coupling | ch1Coupling );
}

} // namespace Hantek
