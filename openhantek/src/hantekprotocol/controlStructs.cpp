// SPDX-License-Identifier: GPL-2.0-or-later

#include <QString>

#include "controlStructs.h"
#include "definitions.h"

namespace Hantek {


ControlSetGain_CH1::ControlSetGain_CH1() : ControlCommand( ControlCode::CONTROL_SETGAIN_CH1, 2 ) { setGainCH1( 1, 7 ); }

void ControlSetGain_CH1::setGainCH1( uint8_t gain, uint8_t index ) {
    data()[ 0 ] = gain;
    data()[ 1 ] = index;
}


ControlSetGain_CH2::ControlSetGain_CH2() : ControlCommand( ControlCode::CONTROL_SETGAIN_CH2, 2 ) { setGainCH2( 1, 7 ); }

void ControlSetGain_CH2::setGainCH2( uint8_t gain, uint8_t index ) {
    data()[ 0 ] = gain;
    data()[ 1 ] = index;
}


ControlSetSamplerate::ControlSetSamplerate() : ControlCommand( ControlCode::CONTROL_SETSAMPLERATE, 2 ) { setSamplerate( 1, 7 ); }

void ControlSetSamplerate::setSamplerate( uint8_t id, uint8_t index ) {
    data()[ 0 ] = id;
    data()[ 1 ] = index;
}


ControlSetNumChannels::ControlSetNumChannels() : ControlCommand( ControlCode::CONTROL_SETNUMCHANNELS, 1 ) { setNumChannels( 2 ); }

void ControlSetNumChannels::setNumChannels( uint8_t val ) { data()[ 0 ] = val; }


ControlStartSampling::ControlStartSampling() : ControlCommand( ControlCode::CONTROL_STARTSAMPLING, 1 ) { data()[ 0 ] = 0x01; }


ControlStopSampling::ControlStopSampling() : ControlCommand( ControlCode::CONTROL_STARTSAMPLING, 1 ) { data()[ 0 ] = 0x00; }


ControlGetCalibration::ControlGetCalibration() : ControlCommand( ControlCode::CONTROL_EEPROM, sizeof( CalibrationValues ) ) {
    value = uint8_t( 8 ); // get calibration values from EEPROM offset 8
    data()[ 0 ] = 0x01;
}


ControlSetCalFreq::ControlSetCalFreq() : ControlCommand( ControlCode::CONTROL_SETCALFREQ, 1 ) {
    setCalFreq( 1 ); // 1kHz
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

const std::vector< QString > controlNames = { "SETGAIN_CH1",    "SETGAIN_CH2", "SETSAMPLERATE", "STARTSAMPLING",
                                              "SETNUMCHANNELS", "SETCOUPLING", "SETCALFREQ" };

} // namespace Hantek
