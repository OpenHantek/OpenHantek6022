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


#include <stdint.h>


#include "helper.h"


#define HANTEK_VENDOR_ID         0x04b5 ///< VID for Hantek DSOs with loaded fw
#define HANTEK_EP_OUT              0x02 ///< OUT Endpoint for bulk transfers
#define HANTEK_EP_IN               0x86 ///< IN Endpoint for bulk transfers
#define HANTEK_TIMEOUT              500 ///< Timeout for USB transfers in ms
#define HANTEK_TIMEOUT_MULTI         10 ///< Timeout for multi packet USB transfers in ms
#define HANTEK_ATTEMPTS               3 ///< The number of transfer attempts
#define HANTEK_ATTEMPTS_MULTI         1 ///< The number of multi packet transfer attempts

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
		///       <td>0x00</td>
		///       <td>FilterBits</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   This command is used by the official %Hantek software, but doesn't seem to be used by the device.
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
		///       <td>Downsampler[0]</td>
		///       <td>Downsampler[1]</td>
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
		///   The samplerate is set relative to the base samplerate by a divider or to a maximum samplerate.<br />
		///   This divider is set by Tsr1Bits.samplerateId for values up to 5 with to the following values:
		///   <table>
		///     <tr>
		///       <td><b>Tsr1Bits.samplerateId</b></td><td>0</td><td>1</td><td>2</td><td>3</td>
		///     </tr>
		///     <tr>
		///       <td><b>Samplerate</b></td><td>Max</td><td>Base</td><td>Base / 2</td><td>Base / 5</td>
		///     </tr>
		///   </table>
		///   For higher divider values, the value can be set using the 16-bit value in the two Downsampler bytes. The value of Downsampler is given by:<br />
		///   <i>Downsampler = 1comp((Base / Samplerate / 2) - 2)</i><br />
		///   The Base samplerate is 50 MS/s for the DSO-2090 and DSO-2150. The Max samplerate is also 50 MS/s for the DSO-2090 and 75 MS/s for the DSO-2150.<br />
		///   When using fast rate mode the Base and Max samplerate is twice as fast. When Tsr1Bits.recordLength is 0 (Roll mode) the sampling rate is divided by 1000.
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
		///       <td>0x00</td>
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
		///       <td>0x00</td>
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

		/// BulkSetChannels2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the activated channels for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0b</td>
		///       <td>0x00</td>
		///       <td>BUsedChannels</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p><br /></p>
		BULK_BSETCHANNELS,

		/// BulkSetTrigger2250 [<em>::MODEL_DSO2250</em>]
		/// <p>
		///   This command sets the trigger source for the DSO-2250:
		///   <table>
		///     <tr>
		///       <td>0x0c</td>
		///       <td>0x00</td>
		///       <td>CTriggerBits</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
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
		///   The samplerate is set relative to the maximum sample rate by a divider that is set in SamplerateFast and the 16-bit value in the two SamplerateSlow bytes.<br />
		///   Without using fast rate mode, the samplerate is:<br />
		///   <i>Samplerate = SamplerateMax / (2comp(SamplerateSlow) * 2 + 4 - SamplerateFast)</i><br />
		///   SamplerateBase is 100 MS/s for the DSO-5200 in normal mode and 200 MS/s in fast rate mode, the modifications regarding record length are the the same that apply for the DSO-2090. The maximum samplerate is 125 MS/s in normal mode and 250 MS/s in fast rate mode, and is reached by setting SamplerateSlow = 0 and SamplerateFast = 4.
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
		///       <td>Samplerate[0]</td>
		///       <td>Samplerate[1]</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The downsampler can be activated by setting ESamplerateBits.downsampling = 1. If this is the case, the value of Downsampler is given by:<br />
		///   <i>Downsampler = 1comp((Base / Samplerate) - 2)</i><br />
		///   Base is 100 MS/s for the DSO-2250 in standard mode and 200 MS/s in fast rate mode, the modifications regarding record length are the the same that apply for the DSO-2090. The maximum samplerate is 125 MS/s in standard mode and 250 MS/s in fast rate mode and is achieved by setting ESamplerateBits.downsampling = 0.
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
		///       <td>TriggerPositionPost[0]</td>
		///       <td>TriggerPositionPost[1]</td>
		///       <td>TriggerPositionPost[2]</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		///   <table>
		///     <tr>
		///       <td>TriggerPositionPre[0]</td>
		///       <td>TriggerPositionPre[1]</td>
		///       <td>TriggerPositionPre[2]</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///       <td>0x00</td>
		///     </tr>
		///   </table>
		/// </p>
		/// <p>
		///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger position. Both values have a range from 0x7d800 (0x00000 for 512 kiS buffer) to 0x7ffff. On the left side (0 %) the TriggerPositionPre value is minimal, on the right side (100 %) it is maximal. The TriggerPositionPost value is maximal for 0 % and minimal for 100%.
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
	/// \enum BUsedChannels                                         hantek/types.h
	/// \brief The enabled channels for the DSO-2250.
	enum BUsedChannels {
		BUSED_CH1,    ///< Only channel 1 is activated
		BUSED_NONE,   ///< No channels are activated
		BUSED_CH1CH2, ///< Channel 1 and 2 are both activated
		BUSED_CH2     ///< Only channel 2 is activated
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum DTriggerPositionUsed                                  hantek/types.h
	/// \brief The trigger position states for the 0x0d command.
	enum DTriggerPositionUsed {
		DTRIGGERPOSITION_OFF = 0, ///< Used for Roll mode
		DTRIGGERPOSITION_ON = 7 ///< Used for normal operation
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct FilterBits                                          hantek/types.h
	/// \brief The bits for BULK_SETFILTER.
	struct FilterBits {
		uint8_t channel1:1; ///< Set to true when channel 1 isn't used
		uint8_t channel2:1; ///< Set to true when channel 2 isn't used
		uint8_t trigger:1; ///< Set to true when trigger isn't used
		uint8_t reserved:5; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct GainBits                                            hantek/types.h
	/// \brief The gain bits for BULK_SETGAIN.
	struct GainBits {
		uint8_t channel1:2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
		uint8_t channel2:2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
		uint8_t reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct Tsr1Bits                                            hantek/types.h
	/// \brief Trigger and samplerate bits (Byte 1).
	struct Tsr1Bits {
		uint8_t triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		uint8_t recordLength:3; ///< See ::RecordLengthId
		uint8_t samplerateId:2; ///< Samplerate ID when downsampler is disabled
		uint8_t downsamplingMode:1; ///< true, if Downsampler is used
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct Tsr2Bits                                            hantek/types.h
	/// \brief Trigger and samplerate bits (Byte 2).
	struct Tsr2Bits {
		uint8_t usedChannels:2; ///< Used channels, see Hantek::UsedChannels
		uint8_t fastRate:1; ///< true, if one channels uses all buffers
		uint8_t triggerSlope:1; ///< The trigger slope, see Dso::Slope, inverted when Tsr1Bits.samplerateFast is uneven
		uint8_t reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct CTriggerBits                                        hantek/types.h
	/// \brief Trigger bits for 0x0c command.
	struct CTriggerBits {
		uint8_t triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		uint8_t triggerSlope:1; ///< The trigger slope, see Dso::Slope
		uint8_t reserved:5; ///< Unused bits
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \struct DBufferBits                                         hantek/types.h
	/// \brief Buffer mode bits for 0x0d command.
	struct DBufferBits {
		uint8_t triggerPositionUsed:3; ///< See ::DTriggerPositionUsed
		uint8_t recordLength:3; ///< See ::RecordLengthId
		uint8_t reserved:2; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ESamplerateBits                                     hantek/types.h
	/// \brief Samplerate bits for DSO-2250 0x0e command.
	struct ESamplerateBits {
		uint8_t fastRate:1; ///< false, if one channels uses all buffers
		uint8_t downsampling:1; ///< true, if the downsampler is activated
		uint8_t reserved:4; ///< Unused bits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ETsrBits                                            hantek/types.h
	/// \brief Trigger and samplerate bits for DSO-5200/DSO-5200A 0x0e command.
	struct ETsrBits {
		uint8_t fastRate:1; ///< false, if one channels uses all buffers
		uint8_t usedChannels:2; ///< Used channels, see Hantek::UsedChannels
		uint8_t triggerSource:2; ///< The trigger source, see Hantek::TriggerSource
		uint8_t triggerSlope:2; ///< The trigger slope, see Dso::Slope
		uint8_t triggerPulse:1; ///< Pulses are causing trigger events
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetFilter                                        hantek/types.h
	/// \brief The BULK_SETFILTER builder.
	class BulkSetFilter : public Helper::DataArray<uint8_t> {
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
	class BulkSetTriggerAndSamplerate : public Helper::DataArray<uint8_t> {
		public:
			BulkSetTriggerAndSamplerate();
			BulkSetTriggerAndSamplerate(uint16_t downsampler, uint32_t triggerPosition, uint8_t triggerSource = 0, uint8_t recordLength = 0, uint8_t samplerateId = 0, bool downsamplingMode = true, uint8_t usedChannels = 0, bool fastRate = false, uint8_t triggerSlope = 0);
			
			uint8_t getTriggerSource();
			void setTriggerSource(uint8_t value);
			uint8_t getRecordLength();
			void setRecordLength(uint8_t value);
			uint8_t getSamplerateId();
			void setSamplerateId(uint8_t value);
			bool getDownsamplingMode();
			void setDownsamplingMode(bool downsampling);
			uint8_t getUsedChannels();
			void setUsedChannels(uint8_t value);
			bool getFastRate();
			void setFastRate(bool fastRate);
			uint8_t getTriggerSlope();
			void setTriggerSlope(uint8_t slope);
			uint16_t getDownsampler();
			void setDownsampler(uint16_t downsampler);
			uint32_t getTriggerPosition();
			void setTriggerPosition(uint32_t position);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkForceTrigger                                     hantek/types.h
	/// \brief The BULK_FORCETRIGGER builder.
	class BulkForceTrigger : public Helper::DataArray<uint8_t> {
		public:
			BulkForceTrigger();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkCaptureStart                                     hantek/types.h
	/// \brief The BULK_CAPTURESTART builder.
	class BulkCaptureStart : public Helper::DataArray<uint8_t> {
		public:
			BulkCaptureStart();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkTriggerEnabled                                   hantek/types.h
	/// \brief The BULK_TRIGGERENABLED builder.
	class BulkTriggerEnabled : public Helper::DataArray<uint8_t> {
		public:
			BulkTriggerEnabled();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetData                                          hantek/types.h
	/// \brief The BULK_GETDATA builder.
	class BulkGetData : public Helper::DataArray<uint8_t> {
		public:
			BulkGetData();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetCaptureState                                  hantek/types.h
	/// \brief The BULK_GETCAPTURESTATE builder.
	class BulkGetCaptureState : public Helper::DataArray<uint8_t> {
		public:
			BulkGetCaptureState();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkResponseGetCaptureState                          hantek/types.h
	/// \brief The parser for the BULK_GETCAPTURESTATE response.
	class BulkResponseGetCaptureState : public Helper::DataArray<uint8_t> {
		public:
			BulkResponseGetCaptureState();
			
			CaptureState getCaptureState();
			unsigned int getTriggerPoint();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetGain                                          hantek/types.h
	/// \brief The BULK_SETGAIN builder.
	class BulkSetGain : public Helper::DataArray<uint8_t> {
		public:
			BulkSetGain();
			BulkSetGain(uint8_t channel1, uint8_t channel2);
			
			uint8_t getGain(unsigned int channel);
			void setGain(unsigned int channel, uint8_t value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetLogicalData                                   hantek/types.h
	/// \brief The BULK_SETLOGICALDATA builder.
	class BulkSetLogicalData : public Helper::DataArray<uint8_t> {
		public:
			BulkSetLogicalData();
			BulkSetLogicalData(uint8_t data);
			
			uint8_t getData();
			void setData(uint8_t data);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkGetLogicalData                                   hantek/types.h
	/// \brief The BULK_GETLOGICALDATA builder.
	class BulkGetLogicalData : public Helper::DataArray<uint8_t> {
		public:
			BulkGetLogicalData();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetChannels2250                                  hantek/types.h
	/// \brief The DSO-2250 BULK_BSETFILTER builder.
	class BulkSetChannels2250 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetChannels2250();
			BulkSetChannels2250(uint8_t usedChannels);
			
			uint8_t getUsedChannels();
			void setUsedChannels(uint8_t value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetTrigger2250                                   hantek/types.h
	/// \brief The DSO-2250 BULK_CSETTRIGGERORSAMPLERATE builder.
	class BulkSetTrigger2250 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetTrigger2250();
			BulkSetTrigger2250(uint8_t triggerSource, uint8_t triggerSlope);
			
			uint8_t getTriggerSource();
			void setTriggerSource(uint8_t value);
			uint8_t getTriggerSlope();
			void setTriggerSlope(uint8_t slope);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetSamplerate5200                                hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_CSETTRIGGERORSAMPLERATE builder.
	class BulkSetSamplerate5200 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetSamplerate5200();
			BulkSetSamplerate5200(uint16_t samplerateSlow, uint8_t samplerateFast);
			
			uint8_t getSamplerateFast();
			void setSamplerateFast(uint8_t value);
			uint16_t getSamplerateSlow();
			void setSamplerateSlow(uint16_t samplerate);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetRecordLength2250                              hantek/types.h
	/// \brief The DSO-2250 BULK_DSETBUFFER builder.
	class BulkSetRecordLength2250 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetRecordLength2250();
			BulkSetRecordLength2250(uint8_t recordLength);
			
			uint8_t getRecordLength();
			void setRecordLength(uint8_t value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetBuffer5200                                    hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_DSETBUFFER builder.
	class BulkSetBuffer5200 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetBuffer5200();
			BulkSetBuffer5200(uint16_t triggerPositionPre, uint16_t triggerPositionPost, uint8_t usedPre = 0, uint8_t usedPost = 0, uint8_t recordLength = 0);
			
			uint16_t getTriggerPositionPre();
			void setTriggerPositionPre(uint16_t value);
			uint16_t getTriggerPositionPost();
			void setTriggerPositionPost(uint16_t value);
			uint8_t getUsedPre();
			void setUsedPre(uint8_t value);
			uint8_t getUsedPost();
			void setUsedPost(uint8_t value);
			uint8_t getRecordLength();
			void setRecordLength(uint8_t value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetSamplerate2250                                hantek/types.h
	/// \brief The DSO-2250 BULK_ESETTRIGGERORSAMPLERATE builder.
	class BulkSetSamplerate2250 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetSamplerate2250();
			BulkSetSamplerate2250(bool fastRate, bool downsampling = false, uint16_t samplerate = 0);
			
			bool getFastRate();
			void setFastRate(bool fastRate);
			bool getDownsampling();
			void setDownsampling(bool downsampling);
			uint16_t getSamplerate();
			void setSamplerate(uint16_t samplerate);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetTrigger5200                                   hantek/types.h
	/// \brief The DSO-5200/DSO-5200A BULK_ESETTRIGGERORSAMPLERATE builder.
	class BulkSetTrigger5200 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetTrigger5200();
			BulkSetTrigger5200(uint8_t triggerSource, uint8_t usedChannels, bool fastRate = false, uint8_t triggerSlope = 0, uint8_t triggerPulse = 0);
			
			uint8_t getTriggerSource();
			void setTriggerSource(uint8_t value);
			uint8_t getUsedChannels();
			void setUsedChannels(uint8_t value);
			bool getFastRate();
			void setFastRate(bool fastRate);
			uint8_t getTriggerSlope();
			void setTriggerSlope(uint8_t slope);
			bool getTriggerPulse();
			void setTriggerPulse(bool pulse);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetBuffer2250                                    hantek/types.h
	/// \brief The DSO-2250 BULK_FSETBUFFER builder.
	class BulkSetBuffer2250 : public Helper::DataArray<uint8_t> {
		public:
			BulkSetBuffer2250();
			BulkSetBuffer2250(uint32_t triggerPositionPre, uint32_t triggerPositionPost);
			
			uint32_t getTriggerPositionPost();
			void setTriggerPositionPost(uint32_t value);
			uint32_t getTriggerPositionPre();
			void setTriggerPositionPre(uint32_t value);
		
		private:
			void init();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlGetSpeed                                      hantek/types.h
	/// \brief The CONTROL_GETSPEED parser.
	class ControlGetSpeed : public Helper::DataArray<uint8_t> {
		public:
			ControlGetSpeed();
			
			ConnectionSpeed getSpeed();
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class ControlBeginCommand                                  hantek/types.h
	/// \brief The CONTROL_BEGINCOMMAND builder.
	class ControlBeginCommand : public Helper::DataArray<uint8_t> {
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
	class ControlSetOffset : public Helper::DataArray<uint8_t> {
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
	class ControlSetRelays : public Helper::DataArray<uint8_t> {
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
