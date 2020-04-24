// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <inttypes.h>

namespace Hantek {

/// \brief All supported control commands.
/// \brief All supported control commands.
/// 0xE0 CONTROL_SETVOLTDIV_CH1 CH1 voltage div setting (6022BE/BL)
///
/// 0xE1 CONTROL_SETVOLTDIV_CH2 CH2 voltage div setting (6022BE/BL)
///
/// CONTROL_SETTIMEDIV Time divisor setting (6022BE/BL)
///
/// CONTROL_ACQUIIRE_HARD_DATA Request sample data (6022BE/BL)
///
/// | Oscilloscope Command | bRequest Value | Other Notes                                                            |
/// |----------------------|----------------|------------------------------------------------------------------------|
/// | Set CH0 voltage range|      0xE0      | Possible values: 1,2,5,10 (5V, 2.5V, 1V, 500mV).                       |
/// | Set CH1 voltage range|      0xE1      | Possible values: 1,2,5,10 (5V, 2.5V, 1V, 500mV).                       |
/// | Set Sampling Rate    |      0xE2      | Possible values: 48, 30, 24, 16, 8, 4, 1 (MHz) and 50,20,10 (*10kHz).  |
/// | Trigger Oscilloscope |      0xE3      | Possible values: 0: start sampling, 1: stop sampling                   |
/// | Set channel count    |      0xE4      | Possible values are 1 (CH1 only) and 2 (CH1 and CH2)                   |
/// | Set Calibration Freq |      0xE6      | Possible values: 1, 2, 5, 10, 20, 50, 100 (kHz)                        |
/// |                      |                | Possible values: 105 (50 Hz), 110 (100 Hz), 120 (200 Hz), 150 (500 Hz) |
/// | Read/Write Firmware  |      0xA0      | Read or write the scope firmware. Must be done on scope initialization |
///
/// The 0xEx requests are sent with value 0x00.
/// The calibration commands are sent with value 0x08 (offset into eeprom).
/// The value for R/W command is dependent on the Cypress protocol for interacting with the firmware.
///
/// A bulk read from end point 0x86 reads the current contents of the FIFO, which the ADC is filling.
///
/// CONTROL_SETVOLTDIV_CH1 CH1 voltage div setting (6022BE/BL)
///
/// CONTROL_SETVOLTDIV_CH2 CH2 voltage div setting (6022BE/BL)
///
/// CONTROL_SETTIMEDIV Time divisor setting (6022BE/BL)
///
/// CONTROL_ACQUIIRE_HARD_DATA Request sample data (6022BE/BL)

enum class ControlCode : uint8_t {
    CONTROL_GETEEPROM = 0xa2,
    CONTROL_SETVOLTDIV_CH1 = 0xe0,
    CONTROL_SETVOLTDIV_CH2 = 0xe1,
    CONTROL_SETTIMEDIV = 0xe2,
    CONTROL_STARTSAMPLING = 0xe3,
    CONTROL_SETNUMCHANNELS = 0xe4,
    CONTROL_SETCOUPLING = 0xe5, // DC/AC not possible without hw modification on Hantek 6022, but implemented on Sainsmart DS120
    CONTROL_SETCALFREQ = 0xe6
};

} // namespace Hantek
