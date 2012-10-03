////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file hantek/types.h
/// \brief Declares types needed for the Hantek::Device class.
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef HANTEK_TYPES_H
#define HANTEK_TYPES_H


#include "helper.h"


#define HANTEK_VENDOR_ID         0x04b5 ///< VID for Hantek DSOs with loaded fw
#define HANTEK_EP_OUT              0x02 ///< OUT Endpoint for bulk transfers
#define HANTEK_EP_IN               0x86 ///< IN Endpoint for bulk transfers
#define HANTEK_TIMEOUT              500 ///< Timeout for USB transfers in ms
#define HANTEK_ATTEMPTS_DEFAULT       3 ///< The number of transfer attempts

#define HANTEK_CHANNELS               2 ///< Number of physical channels
#define HANTEK_SPECIAL_CHANNELS       2 ///< Number of special channels


////////////////////////////////////////////////////////////////////////////////
/// \namespace Hantek                                             hantek/types.h
/// \brief All %Hantek DSO device specific things.
namespace Hantek {
	//////////////////////////////////////////////////////////////////////////////
	/// \enum BulkCode                                              hantek/types.h
	/// \brief All supported bulk commands.
	/// Indicies given in square brackets specify byte numbers in little endian format.
	enum BulkCode {
		/// BulkSetFilter [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets channel and trigger filter:
		///   <table>
		///     <tr>
		///       <td>0x00</td>
		///       <td>0x0f</td>
		///       <td>FilterBits</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_SETFILTER,
		
		/// BulkSetTriggerAndSamplerate [<em>::MODEL_DSO2090, ::MODEL_DSO2150</em>]
		/// <p>
		///   This command sets trigger and timebase:
		///   <table>
		///     <tr>
		///       <td>0x01</td>
		///       <td>0x00</td>
		///       <td>Tsr1Bits</td>
		///       <td>Tsr2Bits</td>
		///       <td>SamplerateSlow[0]</td>
		///       <td>SamplerateSlow[1]</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>TriggerPosition[0]</td>
		///       <td>TriggerPosition[1]</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>TriggerPosition[2]</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The samplerate is set relative to the maximum sample rate by a divider that is set in Tsr1Bits.samplerateFast and the 16-bit value in the two SamplerateSlow bytes.<br />
		///   Without using fast rate mode, the samplerate is:<br />
		///   <i>Samplerate = SamplerateMax / (1comp(SamplerateSlow) * 2 + Tsr1Bits.samplerateFast)</i><br />
		///   SamplerateMax is 50 MHz for the DSO-2090.<br />
		///   When using fast rate mode the resulting samplerate is twice (For DSO-2150 three times) as fast, when using the large buffer it is half as fast. When Tsr1Bits.recordLength is 0 (Roll mode) the sampling rate is divided by 1000. Setting Tsr1Bits.samplerateFast to 0 doesn't work, the result will be the same as Tsr1Bits.samplerateFast = 1. SamplerateSlow can't be used together with fast rate mode, the result is always the the same as SlowValue = 0.
		/// </p>
		/// <p>
		///   The TriggerPosition sets the position of the pretrigger in samples. The left side (0 %) is 0x77660 when using the small buffer and 0x78000 when using the large buffer.
		/// </p>
		/// <p><br /></p>
		BULK_SETTRIGGERANDSAMPLERATE,
		
		/// BulkForceTrigger [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command forces triggering:
		///   <table>
		///     <tr>
		///       <td>0x02</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_FORCETRIGGER,
		
		/// BulkCaptureStart [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command starts to capture data:
		///   <table>
		///     <tr>
		///       <td>0x03</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_STARTSAMPLING,
		
		/// BulkTriggerEnabled [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets the trigger:
		///   <table>
		///     <tr>
		///       <td>0x04</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_ENABLETRIGGER,
		
		/// BulkGetData [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command reads data from the hardware:
		///   <table>
		///     <tr>
		///       <td>0x05</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The oscilloscope returns the sample data, that will be split if it's larger than the IN endpoint packet length:
		///   <table>
		///     <tr>
		///       <td>Sample[0]</td>
		///       <td>...</td>
		///       <td>Sample[511]</td>
		///     </tr>
		///     <tr>
		///       <td>Sample[512]</td>
		///       <td>...</td>
		///       <td>Sample[1023]</td>
		///     </tr>
		///     <tr>
		///       <td>Sample[1024]</td>
		///       <td colspan="2">...</td>
		///     </tr>
		///   </table>
		///   Because of the 10 bit data model, the DSO-5200 transmits the two extra bits for each sample afterwards:
		///   <table>
		///     <tr>
		///       <td>Extra[0] << 2 | Extra[1]</td>
		///       <td>0</td>
		///       <td>Extra[2] << 2 | Extra[3]</td>
		///       <td>0</td>
		///       <td>...</td>
		///       <td>Extra[510] << 2 | Extra[511]</td>
		///       <td>0</td>
		///     </tr>
		///     <tr>
		///       <td>Extra[512] << 2 | Extra[513]</td>
		///       <td colspan="6">...</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_GETDATA,
		
		/// BulkGetCaptureState [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command checks the capture state:
		///   <table>
		///     <tr>
		///       <td>0x06</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The oscilloscope returns it's capture state and the trigger point. Not sure about this, looks like 248 16-bit words with nearly constant values. These can be converted to the start address of the data in the buffer (See Hantek::Control::calculateTriggerPoint):
		///   <table>
		///     <tr>
		///       <td>::CaptureState</td>
		///       <td>0x00</td>
		///       <td>TriggerPoint[0]</td>
		///       <td>TriggerPoint[1]</td>
		///       <td>...</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_GETCAPTURESTATE,
		
		/// BulkSetGain [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets the gain:
		///   <table>
		///     <tr>
		///       <td>0x07</td>
		///       <td>0x0f</td>
		///       <td>GainBits</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		///   It is usually used in combination with ::CONTROL_SETRELAYS.
		/// </p>
		/// <p><br /></p>
		BULK_SETGAIN,
		
		/// BulkSetLogicalData [<em></em>]
		/// <p>
		///   This command sets the logical data (Not used in official %Hantek software):
		///   <table>
		///     <tr>
		///       <td>0x08</td>
		///       <td>0x0f</td>
		///       <td>Data | 0x01</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_SETLOGICALDATA,
		
		/// BulkGetLogicalData [<em></em>]
		/// <p>
		///   This command reads the logical data (Not used in official %Hantek software):
		///   <table>
		///     <tr>
		///       <td>0x09</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The oscilloscope returns the logical data, which contains valid data in the first byte although it is 64 or 512 bytes long:
		///   <table>
		///     <tr>
		///       <td>Data</td>
		///       <td>...</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_GETLOGICALDATA,
		
		/// [<em></em>]
		/// <p>
		///   This command isn't used for any supported model:
		///   <table>
		///     <tr>
		///       <td>0x0a</td>
		///       <td>...</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_AUNKNOWN,

		/// BulkSetFilter2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the activated channels for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0b</td>
		///       <td>0x00</td>
		///       <td>FilterBits</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_BSETFILTER,

		/// BulkSetTrigger2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the trigger source for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0c</td>
		///       <td>0x0f</td>
		///       <td>CTriggerBits</td>
		///       <td>0x00</td>
		///       <td>0x02</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		/// BulkSetSamplerate5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets the sampling rate for the DSO-5200:
		///   <table>
		///     <tr>
		///       <td>0x0c</td>
		///       <td>0x00</td>
		///       <td>SamplerateSlow[0]</td>
		///       <td>SamplerateSlow[1]</td>
		///       <td>SamplerateFast</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The values are similar to the ones used with ::BULK_SETTRIGGERANDSAMPLERATE. The formula is a bit different here:<br />
		///   <i>Samplerate = SamplerateMax / (2comp(SamplerateSlow) * 2 + 4 - SamplerateFast)</i><br />
		///   SamplerateMax is 100 MS/s for the DSO-5200 in default configuration and 250 MS/s in fast rate mode though, the modifications regarding record length are the the same that apply for the DSO-2090.
		/// </p>
		/// <p><br /></p>
		BULK_CSETTRIGGERORSAMPLERATE,

		/// BulkSetRecordLength2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the record length for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0d</td>
		///       <td>0x00</td>
		///       <td>::RecordLengthId</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		/// BulkSetBuffer5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets the trigger position and record length for the DSO-5200:
		///   <table>
		///     <tr>
		///       <td>0x0d</td>
		///       <td>0x00</td>
		///       <td>TriggerPositionPre[0]</td>
		///       <td>TriggerPositionPre[1]</td>
		///       <td>::DTriggerPositionUsed</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>0xff</td>
		///       <td>TriggerPositionPost[0]</td>
		///       <td>TriggerPositionPost[1]</td>
		///       <td>DBufferBits</td>
		///       <td>0xff</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger position. Both values have a range from 0xd7ff (0xc7ff for 14 kiS buffer) to 0xfffe. On the left side (0 %) the TriggerPositionPre value is minimal, on the right side (100 %) it is maximal. The TriggerPositionPost value is maximal for 0 % and minimal for 100%.
		/// </p>
		/// <p><br /></p>
		BULK_DSETBUFFER,
		
		/// BulkSetSamplerate2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the samplerate:
		///   <table>
		///     <tr>
		///       <td>0x0e</td>
		///       <td>0x00</td>
		///       <td>ESamplerateBits</td>
		///       <td>0x00</td>
		///       <td>SamplerateSlow[0]</td>
		///       <td>SamplerateSlow[1]</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The values are similar to the ones used with ::BULK_SETTRIGGERANDSAMPLERATE. The formula is a bit different here:<br />
		///   <i>Samplerate = SamplerateMax / (2comp(SamplerateSlow) * 2 + ESamplerateBits.samplerateFast)</i><br />
		///   SamplerateMax is 100 MS/s for the DSO-2250 in default configuration and 250 MS/s in fast rate mode though, the modifications regarding record length are the the same that apply for the DSO-2090.
		/// </p>
		/// <p><br /></p>
		/// BulkSetTrigger5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
		/// <p>
		///   This command sets the channel and trigger settings:
		///   <table>
		///     <tr>
		///       <td>0x0e</td>
		///       <td>0x00</td>
		///       <td>ETsrBits</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_ESETTRIGGERORSAMPLERATE,

		/// BulkSetBuffer2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the trigger position and buffer configuration for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0f</td>
		///       <td>0x00</td>
		///       <td>TriggerPositionPre[0]</td>
		///       <td>TriggerPositionPre[1]</td>
		///       <td>FBuffer1Bits</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>TriggerPositionPost[0]</td>
		///       <td>TriggerPositionPost[1]</td>
		///       <td>FBuffer1Bits</td>
		///       <td>0x00</td>
		///       <td>FBuffer2Bits</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger position. Both values have a range from 0xd7ff (0xc7ff for 14 kiS buffer) to 0xfffe. On the left side (0 %) the TriggerPositionPre value is minimal, on the right side (100 %) it is maximal. The TriggerPositionPost value is maximal for 0 % and minimal for 100%.
		/// </p>
		/// <p><br /></p>
		BULK_FSETBUFFER,

		BULK_COUNT
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ControlCode                                           hantek/types.h
	/// \brief All supported control commands.
	enum ControlCode {
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		///   The 0xa2 control read/write command gives access to a ::ControlValue.
		/// </p>
		/// <p><br /></p>
		CONTROL_VALUE = 0xa2,
		
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		///   The 0xb2 control read command gets the speed level of the USB connection:
		///   <table>
		///     <tr>
		///       <td>::ConnectionSpeed</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		CONTROL_GETSPEED = 0xb2,
		
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		///   The 0xb3 control write command is sent before any bulk command:
		///   <table>
		///     <tr>
		///       <td>0x0f</td>
		///       <td>::BulkIndex</td>
		///       <td>::BulkIndex</td>
		///       <td>::BulkIndex</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		CONTROL_BEGINCOMMAND = 0xb3,
		
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		///   The 0xb4 control write command sets the channel offsets:
		///   <table>
		///     <tr>
		///       <td>Ch1Offset[1]</td>
		///       <td>Ch1Offset[0]</td>
		///       <td>Ch2Offset[1]</td>
		///       <td>Ch2Offset[0]</td>
		///       <td>TriggerOffset[1]</td>
		///       <td>TriggerOffset[0]</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		CONTROL_SETOFFSET = 0xb4,
		
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		///   The 0xb5 control write command sets the internal relays:
		///   <table>
		///     <tr>
		///       <td>0x00</td>
		///       <td>0x04 ^ (Ch1Gain < 1 V)</td>
		///       <td>0x08 ^ (Ch1Gain < 100 mV)</td>
		///       <td>0x02 ^ (Ch1Coupling == DC)</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>0x20 ^ (Ch2Gain < 1 V)</td>
		///       <td>0x40 ^ (Ch2Gain < 100 mV)</td>
		///       <td>0x10 ^ (Ch2Coupling == DC)</td>
		///       <td>0x01 ^ (Trigger == EXT)</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The limits are <= instead of < for the 10 bit models, since those support voltages up to 10 V.
		/// </p>
		/// <p><br /></p>
		CONTROL_SETRELAYS = 0xb5
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ControlValue                                          hantek/types.h
	/// \brief All supported values for control commands.
	enum ControlValue {
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		/// Value 0x08 is the calibration data for the channels offsets. It holds the offset value for the top and bottom of the scope screen for every gain step on every channel. The data is stored as a three-dimensional array:<br />
		/// <i>channelLevels[channel][GainId][::LevelOffset]</i>
		/// </p>
		/// <p><br /></p>
		VALUE_OFFSETLIMITS = 0x08,
		
		/// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		/// Value 0x0a is the address of the device. It has a length of one byte.
		/// </p>
		/// <p><br /></p>
		VALUE_DEVICEADDRESS = 0x0a,
		
		/// <em>[::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		/// Value 0x60 is the calibration data for the fast rate mode on the DSO-2250, DSO-5200 and DSO-5200A. It's used to correct the level differences between the two merged channels to avoid deterministic noise.
		/// </p>
		/// <p><br /></p>
		VALUE_FASTRATECALIBRATION = 0x60,
		
		/// <em>[::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
		/// <p>
		/// Value 0x70 contains correction values for the ETS functionality of the DSO-5200 and DSO-5200A.
		/// </p>
		/// <p><br /></p>
		VALUE_ETSCORRECTION = 0x70
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum Model                                                 hantek/types.h
	/// \brief All supported Hantek DSO models.
	enum Model {
		MODEL_UNKNOWN = -1, ///< Unknown model
		MODEL_DSO2090,      ///< %Hantek DSO-2090 USB
		MODEL_DSO2150,      ///< %Hantek DSO-2150 USB
		MODEL_DSO2250,      ///< %Hantek DSO-2250 USB
		MODEL_DSO5200,      ///< %Hantek DSO-5200 USB
		MODEL_DSO5200A,     ///< %Hantek DSO-5200A USB
		MODEL_COUNT
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ConnectionSpeed                                       hantek/types.h
	/// \brief The speed level of the USB connection.
	enum ConnectionSpeed {
		CONNECTION_FULLSPEED = 0, ///< FullSpeed USB, 64 byte bulk transfers
		CONNECTION_HIGHSPEED = 1  ///< HighSpeed USB, 512 byte bulk transfers
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum UsedChannels                                          hantek/types.h
	/// \brief The enabled channels.
	enum UsedChannels {
		USED_CH1,    ///< Only channel 1 is activated
		USED_CH2,    ///< Only channel 2 is activated
		USED_CH1CH2, ///< Channel 1 and 2 are both activated
		USED_NONE    ///< No channels are activated
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum TriggerSource                                         hantek/types.h
	/// \brief The possible trigger sources.
	enum TriggerSource {
		TRIGGER_CH2, TRIGGER_CH1,
		TRIGGER_ALT,
		TRIGGER_EXT, TRIGGER_EXT10
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum RecordLengthId                                        hantek/types.h
	/// \brief The size id for CommandSetTriggerAndSamplerate.
	enum RecordLengthId {
		RECORDLENGTHID_ROLL = 0, ///< Used for the roll mode
		RECORDLENGTHID_SMALL, ///< The standard buffer with 10240 samples
		RECORDLENGTHID_LARGE ///< The large buffer, 32768 samples (14336 for DSO-5200)
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum CaptureState                                          hantek/types.h
	/// \brief The different capture states which the oscilloscope returns.
	enum CaptureState {
		CAPTURE_WAITING = 0, ///< The scope is waiting for a trigger event
		CAPTURE_SAMPLING = 1, ///< The scope is sampling data after triggering
		CAPTURE_READY = 2, ///< Sampling data is available (DSO-2090/DSO-2150)
		CAPTURE_READY2250 = 3, ///< Sampling data is available (DSO-2250)
		CAPTURE_READY5200 = 7 ///< Sampling data is available (DSO-5200/DSO-5200A)
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum BulkIndex                                             hantek/types.h
	/// \brief Can be set by CONTROL_BEGINCOMMAND, maybe it allows multiple commands at the same time?
	enum BulkIndex {
		COMMANDINDEX_0 = 0x03, ///< Used most of the time
		COMMANDINDEX_1 = 0x0a,
		COMMANDINDEX_2 = 0x09,
		COMMANDINDEX_3 = 0x01, ///< Used for ::BULK_SETTRIGGERANDSAMPLERATE sometimes
		COMMANDINDEX_4 = 0x02,
		COMMANDINDEX_5 = 0x08
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum LevelOffset                                           hantek/types.h
	/// \brief The array indicies for the CalibrationData.
	enum LevelOffset {
		OFFSET_START, ///< The channel level at the bottom of the scope
		OFFSET_END, ///< The channel level at the top of the scope
		OFFSET_COUNT
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum DTriggerPositionUsed                                  hantek/types.h
	/// \brief The trigger position states for the 0x0d command.
	enum DTriggerPositionUsed {
		DTRIGGERPOSITION_OFF = 0, ///< Used for Roll mode
		DTRIGGERPOSITION_ON = 7 ///< Used for normal operation
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum FTriggerPositionUsed                                  hantek/types.h
	/// \brief The trigger position states for the 0x0f command.
	enum FTriggerPositionUsed {
		FTRIGGERPOSITION_OFF = 0, ///< Used for Roll mode
		FTRIGGERPOSITION_ON = 3 ///< Used for normal operation
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct FilterBits                                          hantek/types.h
	/// \brief The bits for BULK_SETFILTER.
	struct FilterBits {
		unsigned char channel1:1; ///< Set to true when channel 1 isn't used
		unsigned char channel2:1; ///< Set to true when channel 2 isn't used
		unsigned char trigger:1; ///< Set to true when trigger isn't used
		unsigned char reserved:5; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct GainBits                                            hantek/types.h
	/// \brief The gain bits for BULK_SETGAIN.
	struct GainBits {
		unsigned char channel1:2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
		unsigned char channel2:2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
		unsigned char reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct Tsr1Bits                                            hantek/types.h
	/// \brief Trigger and samplerate bits (Byte 1).
	struct Tsr1Bits {
		unsigned char triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		unsigned char recordLength:3; ///< See ::RecordLengthId
		unsigned char samplerateFast:3; ///< samplerate value for fast sampling rates
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct Tsr2Bits                                            hantek/types.h
	/// \brief Trigger and samplerate bits (Byte 2).
	struct Tsr2Bits {
		unsigned char usedChannels:2; ///< Used channels, see Hantek::UsedChannels
		unsigned char fastRate:1; ///< true, if one channels uses all buffers
		unsigned char triggerSlope:1; ///< The trigger slope, see Dso::Slope, inverted when Tsr1Bits.samplerateFast is uneven
		unsigned char reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct CTriggerBits                                        hantek/types.h
	/// \brief Trigger bits for 0x0c command.
	struct CTriggerBits {
		unsigned char triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		unsigned char triggerSlope:1; ///< The trigger slope, see Dso::Slope
		unsigned char reserved:5; ///< Unused bits
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \struct DBufferBits                                         hantek/types.h
	/// \brief Buffer mode bits for 0x0d command.
	struct DBufferBits {
		unsigned char triggerPositionUsed:3; ///< See ::DTriggerPositionUsed
		unsigned char recordLength:3; ///< See ::RecordLengthId
		unsigned char reserved:2; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ESamplerateBits                                     hantek/types.h
	/// \brief Samplerate bits for DSO-2250 0x0e command.
	struct ESamplerateBits {
		unsigned char fastRate:1; ///< false, if one channels uses all buffers
		unsigned char samplerateFast:3; ///< samplerate value for fast sampling rates
		unsigned char reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ETsrBits                                            hantek/types.h
	/// \brief Trigger and samplerate bits for DSO-5200/DSO-5200A 0x0e command.
	struct ETsrBits {
		unsigned char fastRate:1; ///< false, if one channels uses all buffers
		unsigned char usedChannels:2; ///< Used channels, see Hantek::UsedChannels
		unsigned char triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		unsigned char triggerSlope:2; ///< The trigger slope, see Dso::Slope
		unsigned char triggerPulse:1; ///< Pulses are causing trigger events
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct FBuffer1Bits                                        hantek/types.h
	/// \brief Buffer mode bits for 0x0f command (Byte 1).
	struct FBuffer1Bits {
		unsigned char triggerPositionUsed:2; ///< See ::DTriggerPositionUsed
		unsigned char largeBuffer:1; ///< false, if ::RecordLengthId is ::RECORDLENGTHID_LARGE
		unsigned char reserved:5; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct FBuffer2Bits                                        hantek/types.h
	/// \brief Buffer mode bits for 0x0f command (Byte 2).
	struct FBuffer2Bits {
		unsigned char reserved:7; ///< Unused bits
		unsigned char slowBuffer:1; ///< false, if ::RecordLengthId is ::RECORDLENGTHID_SMALL
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetFilter                                        hantek/types.h
	/// \brief The BULK_SETFILTER builder.
	class BulkSetFilter : public Helper::DataArray<unsigned char> {
		public:
			BulkSetFilter();
			BulkSetFilter(bool channel1, bool channel2, bool trigger);
			
			bool getChannel(unsigned int channel);
			void setChannel(unsigned int channel, bool filtered);
			bool getTrigger();
			void setTrigger(bool filtered);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetTriggerAndSamplerate                          hantek/types.h
	/// \brief The BULK_SETTRIGGERANDSAMPLERATE builder.
	class BulkSetTriggerAndSamplerate : public Helper::DataArray<unsigned char> {
		public:
			BulkSetTriggerAndSamplerate();
			BulkSetTriggerAndSamplerate(unsigned short int samplerateSlow, unsigned long int triggerPosition, unsigned char triggerSource = 0, unsigned char recordLength = 0, unsigned char samplerateFast = 0, unsigned char usedChannels = 0, bool fastRate = false, unsigned char triggerSlope = 0);
			
			unsigned char getTriggerSource();
			void setTriggerSource(unsigned char value);
			unsigned char getRecordLength();
			void setRecordLength(unsigned char value);
			unsigned char getSamplerateFast();
			void setSamplerateFast(unsigned char value);
			unsigned char getUsedChannels();
			void setUsedChannels(unsigned char value);
			bool getFastRate();
			void setFastRate(bool fastRate);
			unsigned char getTriggerSlope();
			void setTriggerSlope(unsigned char slope);
			unsigned short int getSamplerateSlow();
			void setSamplerateSlow(unsigned short int samplerate);
			unsigned long int getTriggerPosition();
			void setTriggerPosition(unsigned long int position);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkForceTrigger                                     hantek/types.h
	/// \brief The BULK_FORCETRIGGER builder.
	class BulkForceTrigger : public Helper::DataArray<unsigned char> {
		public:
			BulkForceTrigger();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkCaptureStart                                     hantek/types.h
	/// \brief The BULK_CAPTURESTART builder.
	class BulkCaptureStart : public Helper::DataArray<unsigned char> {
		public:
			BulkCaptureStart();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkTriggerEnabled                                   hantek/types.h
	/// \brief The BULK_TRIGGERENABLED builder.
	class BulkTriggerEnabled : public Helper::DataArray<unsigned char> {
		public:
			BulkTriggerEnabled();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetData                                          hantek/types.h
	/// \brief The BULK_GETDATA builder.
	class BulkGetData : public Helper::DataArray<unsigned char> {
		public:
			BulkGetData();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetCaptureState                                  hantek/types.h
	/// \brief The BULK_GETCAPTURESTATE builder.
	class BulkGetCaptureState : public Helper::DataArray<unsigned char> {
		public:
			BulkGetCaptureState();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkResponseGetCaptureState                          hantek/types.h
	/// \brief The parser for the BULK_GETCAPTURESTATE response.
	class BulkResponseGetCaptureState : public Helper::DataArray<unsigned char> {
		public:
			BulkResponseGetCaptureState();
			
			CaptureState getCaptureState();
			unsigned int getTriggerPoint();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetGain                                          hantek/types.h
	/// \brief The BULK_SETGAIN builder.
	class BulkSetGain : public Helper::DataArray<unsigned char> {
		public:
			BulkSetGain();
			BulkSetGain(unsigned char channel1, unsigned char channel2);
			
			unsigned char getGain(unsigned int channel);
			void setGain(unsigned int channel, unsigned char value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetLogicalData                                   hantek/types.h
	/// \brief The BULK_SETLOGICALDATA builder.
	class BulkSetLogicalData : public Helper::DataArray<unsigned char> {
		public:
			BulkSetLogicalData();
			BulkSetLogicalData(unsigned char data);
			
			unsigned char getData();
			void setData(unsigned char data);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetLogicalData                                   hantek/types.h
	/// \brief The BULK_GETLOGICALDATA builder.
	class BulkGetLogicalData : public Helper::DataArray<unsigned char> {
		public:
			BulkGetLogicalData();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetFilter2250                                    hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_BSETFILTER builder.
	class BulkSetFilter2250 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetFilter2250();
			BulkSetFilter2250(bool channel1, bool channel2);
			
			bool getChannel(unsigned int channel);
			void setChannel(unsigned int channel, bool filtered);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetTrigger2250                                   hantek/types.h
	/// \brief The DSO-2250 BULK_CSETTRIGGERORSAMPLERATE builder.
	class BulkSetTrigger2250 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetTrigger2250();
			BulkSetTrigger2250(unsigned char triggerSource, unsigned char triggerSlope);
			
			unsigned char getTriggerSource();
			void setTriggerSource(unsigned char value);
			unsigned char getTriggerSlope();
			void setTriggerSlope(unsigned char slope);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetSamplerate5200                                hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_CSETTRIGGERORSAMPLERATE builder.
	class BulkSetSamplerate5200 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetSamplerate5200();
			BulkSetSamplerate5200(unsigned short int samplerateSlow, unsigned char samplerateFast);
			
			unsigned char getSamplerateFast();
			void setSamplerateFast(unsigned char value);
			unsigned short int getSamplerateSlow();
			void setSamplerateSlow(unsigned short int samplerate);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetRecordLength2250                              hantek/types.h
	/// \brief The DSO-2250 BULK_DSETBUFFER builder.
	class BulkSetRecordLength2250 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetRecordLength2250();
			BulkSetRecordLength2250(unsigned char recordLength);
			
			unsigned char getRecordLength();
			void setRecordLength(unsigned char value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetBuffer5200                                    hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_DSETBUFFER builder.
	class BulkSetBuffer5200 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetBuffer5200();
			BulkSetBuffer5200(unsigned short int triggerPositionPre, unsigned short int triggerPositionPost, unsigned char usedPre = 0, unsigned char usedPost = 0, unsigned char recordLength = 0);
			
			unsigned short int getTriggerPositionPre();
			void setTriggerPositionPre(unsigned short int value);
			unsigned short int getTriggerPositionPost();
			void setTriggerPositionPost(unsigned short int value);
			unsigned char getUsedPre();
			void setUsedPre(unsigned char value);
			unsigned char getUsedPost();
			void setUsedPost(unsigned char value);
			unsigned char getRecordLength();
			void setRecordLength(unsigned char value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetSamplerate2250                                hantek/types.h
	/// \brief The DSO-2250 BULK_ESETTRIGGERORSAMPLERATE builder.
	class BulkSetSamplerate2250 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetSamplerate2250();
			BulkSetSamplerate2250(bool fastRate, unsigned char samplerateFast = 0, unsigned short int samplerateSlow = 0);
			
			bool getFastRate();
			void setFastRate(bool fastRate);
			unsigned char getSamplerateFast();
			void setSamplerateFast(unsigned char value);
			unsigned short int getSamplerateSlow();
			void setSamplerateSlow(unsigned short int samplerate);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetTrigger5200                                   hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_ESETTRIGGERORSAMPLERATE builder.
	class BulkSetTrigger5200 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetTrigger5200();
			BulkSetTrigger5200(unsigned char triggerSource, unsigned char usedChannels, bool fastRate = false, unsigned char triggerSlope = 0, unsigned char triggerPulse = 0);
			
			unsigned char getTriggerSource();
			void setTriggerSource(unsigned char value);
			unsigned char getUsedChannels();
			void setUsedChannels(unsigned char value);
			bool getFastRate();
			void setFastRate(bool fastRate);
			unsigned char getTriggerSlope();
			void setTriggerSlope(unsigned char slope);
			bool getTriggerPulse();
			void setTriggerPulse(bool pulse);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetBuffer2250                                    hantek/types.h
	/// \brief The DSO-2250 BULK_FSETBUFFER builder.
	class BulkSetBuffer2250 : public Helper::DataArray<unsigned char> {
		public:
			BulkSetBuffer2250();
			BulkSetBuffer2250(unsigned short int triggerPositionPre, unsigned short int triggerPositionPost, unsigned char usedPre = 0, unsigned char usedPost = 0, bool largeBuffer = false, bool slowBuffer = false);
			
			unsigned short int getTriggerPositionPre();
			void setTriggerPositionPre(unsigned short int value);
			unsigned short int getTriggerPositionPost();
			void setTriggerPositionPost(unsigned short int value);
			unsigned char getUsedPre();
			void setUsedPre(unsigned char value);
			unsigned char getUsedPost();
			void setUsedPost(unsigned char value);
			bool getLargeBuffer();
			void setLargeBuffer(bool largeBuffer);
			bool getSlowBuffer();
			void setSlowBuffer(bool slowBuffer);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlGetSpeed                                      hantek/types.h
	/// \brief The CONTROL_GETSPEED parser.
	class ControlGetSpeed : public Helper::DataArray<unsigned char> {
		public:
			ControlGetSpeed();
			
			ConnectionSpeed getSpeed();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlBeginCommand                                  hantek/types.h
	/// \brief The CONTROL_BEGINCOMMAND builder.
	class ControlBeginCommand : public Helper::DataArray<unsigned char> {
		public:
			ControlBeginCommand(BulkIndex index = COMMANDINDEX_0);
			
			BulkIndex getIndex();
			void setIndex(BulkIndex index);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlSetOffset                                     hantek/types.h
	/// \brief The CONTROL_SETOFFSET builder.
	class ControlSetOffset : public Helper::DataArray<unsigned char> {
		public:
			ControlSetOffset();
			ControlSetOffset(unsigned short int channel1, unsigned short int channel2, unsigned short int trigger);
			
			unsigned short int getChannel(unsigned int channel);
			void setChannel(unsigned int channel, unsigned short int offset);
			unsigned short int getTrigger();
			void setTrigger(unsigned short int level);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlSetRelays                                     hantek/types.h
	/// \brief The CONTROL_SETRELAYS builder.
	class ControlSetRelays : public Helper::DataArray<unsigned char> {
		public:
			ControlSetRelays(bool ch1Below1V = false, bool ch1Below100mV = false, bool ch1CouplingDC = false, bool ch2Below1V = false, bool ch2Below100mV = false, bool ch2CouplingDC = false, bool triggerExt = false);
			
			bool getBelow1V(unsigned int channel);
			void setBelow1V(unsigned int channel, bool below);
			bool getBelow100mV(unsigned int channel);
			void setBelow100mV(unsigned int channel, bool below);
			bool getCoupling(unsigned int channel);
			void setCoupling(unsigned int channel, bool dc);
			bool getTrigger();
			void setTrigger(bool ext);
	};
}


#endif
