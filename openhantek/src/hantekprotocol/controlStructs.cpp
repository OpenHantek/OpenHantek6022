// SPDX-License-Identifier: GPL-2.0+

#include <cstring>

#include "controlStructs.h"
#include "controlvalue.h"
#include "definitions.h"

namespace Hantek {

ControlBeginCommand::ControlBeginCommand( CommandIndex index )
    : ControlCommand( Hantek::ControlCode::CONTROL_BEGINCOMMAND, 10 ) {
    data()[ 0 ] = 0x0f;
    data()[ 1 ] = uint8_t( index );
}


ControlGetSpeed::ControlGetSpeed() : ControlCommand( Hantek::ControlCode::CONTROL_GETSPEED, 10 ) {}

// ConnectionSpeed ControlGetSpeed::getSpeed() { return (ConnectionSpeed)data()[0]; }


ControlSetVoltDIV_CH1::ControlSetVoltDIV_CH1() : ControlCommand( ControlCode::CONTROL_SETVOLTDIV_CH1, 1 ) {
    this->setDiv( 5 );
}

void ControlSetVoltDIV_CH1::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetVoltDIV_CH2::ControlSetVoltDIV_CH2() : ControlCommand( ControlCode::CONTROL_SETVOLTDIV_CH2, 1 ) {
    this->setDiv( 5 );
}

void ControlSetVoltDIV_CH2::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetTimeDIV::ControlSetTimeDIV() : ControlCommand( ControlCode::CONTROL_SETTIMEDIV, 1 ) { this->setDiv( 1 ); }

void ControlSetTimeDIV::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlSetNumChannels::ControlSetNumChannels() : ControlCommand( ControlCode::CONTROL_SETNUMCHANNELS, 1 ) {
    this->setDiv( 2 );
}

void ControlSetNumChannels::setDiv( uint8_t val ) { data()[ 0 ] = val; }


ControlAcquireHardData::ControlAcquireHardData() : ControlCommand( ControlCode::CONTROL_ACQUIIRE_HARD_DATA, 1 ) {
    data()[ 0 ] = 0x01;
}


ControlGetLimits::ControlGetLimits() : ControlCommand( ControlCode::CONTROL_VALUE, sizeof( CalibrationValues ) ) {
    value = uint8_t( ControlValue::VALUE_OFFSETLIMITS );
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
