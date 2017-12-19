// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaType>
#include <QString>
#include <stdint.h>

#define HANTEK_EP_OUT 0x02 ///< OUT Endpoint for bulk transfers
#define HANTEK_EP_IN 0x86  ///< IN Endpoint for bulk transfers

#define HANTEK_CHANNELS 2         ///< Number of physical channels
#define HANTEK_SPECIAL_CHANNELS 2 ///< Number of special channels

#define MARKER_COUNT 2 ///< Number of markers

/// \namespace Dso
/// \brief All DSO specific things for different modes and so on.
namespace Dso {
/// \enum ErrorCode                                           hantek/control.h
/// \brief The return codes for device control methods.
enum class ErrorCode {
    ERROR_NONE = 0,         ///< Successful operation
    ERROR_CONNECTION = -1,  ///< Device not connected or communication error
    ERROR_UNSUPPORTED = -2, ///< Not supported by this device
    ERROR_PARAMETER = -3    ///< Parameter out of range
};

/// \enum ChannelMode
/// \brief The channel display modes.
enum ChannelMode {
    CHANNELMODE_VOLTAGE,  ///< Standard voltage view
    CHANNELMODE_SPECTRUM, ///< Spectrum view
    CHANNELMODE_COUNT     ///< The total number of modes
};

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    GRAPHFORMAT_TY,   ///< The standard mode
    GRAPHFORMAT_XY,   ///< CH1 on X-axis, CH2 on Y-axis
    GRAPHFORMAT_COUNT ///< The total number of formats
};

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum Coupling {
    COUPLING_AC,   ///< Offset filtered out by condensator
    COUPLING_DC,   ///< No filtering
    COUPLING_GND,  ///< Channel is grounded
    COUPLING_COUNT ///< The total number of coupling modes
};

/// \enum MathMode
/// \brief The different math modes for the math-channel.
enum MathMode {
    MATHMODE_1ADD2, ///< Add the values of the channels
    MATHMODE_1SUB2, ///< Subtract CH2 from CH1
    MATHMODE_2SUB1, ///< Subtract CH1 from CH2
    MATHMODE_COUNT  ///< The total number of math modes
};

/// \enum TriggerMode
/// \brief The different triggering modes.
enum TriggerMode {
    TRIGGERMODE_AUTO,     ///< Automatic without trigger event
    TRIGGERMODE_NORMAL,   ///< Normal mode
    TRIGGERMODE_SINGLE,   ///< Stop after the first trigger event
    TRIGGERMODE_SOFTWARE, ///< Software trigger mode
    TRIGGERMODE_COUNT     ///< The total number of modes
};

/// \enum Slope
/// \brief The slope that causes a trigger.
enum Slope {
    SLOPE_POSITIVE, ///< From lower to higher voltage
    SLOPE_NEGATIVE, ///< From higher to lower voltage
    SLOPE_COUNT     ///< Total number of trigger slopes
};

/// \enum WindowFunction
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum WindowFunction {
    WINDOW_RECTANGULAR,  ///< Rectangular window (aka Dirichlet)
    WINDOW_HAMMING,      ///< Hamming window
    WINDOW_HANN,         ///< Hann window
    WINDOW_COSINE,       ///< Cosine window (aka Sine)
    WINDOW_LANCZOS,      ///< Lanczos window (aka Sinc)
    WINDOW_BARTLETT,     ///< Bartlett window (Endpoints == 0)
    WINDOW_TRIANGULAR,   ///< Triangular window (Endpoints != 0)
    WINDOW_GAUSS,        ///< Gauss window (simga = 0.4)
    WINDOW_BARTLETTHANN, ///< Bartlett-Hann window
    WINDOW_BLACKMAN,     ///< Blackman window (alpha = 0.16)
    // WINDOW_KAISER,                      ///< Kaiser window (alpha = 3.0)
    WINDOW_NUTTALL,         ///< Nuttall window, cont. first deriv.
    WINDOW_BLACKMANHARRIS,  ///< Blackman-Harris window
    WINDOW_BLACKMANNUTTALL, ///< Blackman-Nuttall window
    WINDOW_FLATTOP,         ///< Flat top window
    WINDOW_COUNT            ///< Total number of window functions
};

/// \enum InterpolationMode
/// \brief The different interpolation modes for the graphs.
enum InterpolationMode {
    INTERPOLATION_OFF = 0, ///< Just dots for each sample
    INTERPOLATION_LINEAR,  ///< Sample dots connected by lines
    INTERPOLATION_SINC,    ///< Smooth graph through the dots
    INTERPOLATION_COUNT    ///< Total number of interpolation modes
};
}

Q_DECLARE_METATYPE(Dso::TriggerMode)
Q_DECLARE_METATYPE(Dso::MathMode)
Q_DECLARE_METATYPE(Dso::Slope)
Q_DECLARE_METATYPE(Dso::Coupling)
Q_DECLARE_METATYPE(Dso::GraphFormat)
Q_DECLARE_METATYPE(Dso::ChannelMode)
Q_DECLARE_METATYPE(Dso::WindowFunction)
Q_DECLARE_METATYPE(Dso::InterpolationMode)

////////////////////////////////////////////////////////////////////////////////
/// \namespace Hantek                                             hantek/types.h
/// \brief All %Hantek DSO device specific things.
namespace Hantek {
//////////////////////////////////////////////////////////////////////////////
/// \enum BulkCode                                              hantek/types.h
/// \brief All supported bulk commands.
/// Indicies given in square brackets specify byte numbers in little endian
/// format.
enum BulkCode {
    /// BulkSetFilter [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A</em>]
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
    ///   This command is used by the official %Hantek software, but doesn't seem
    ///   to be used by the device.
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
    ///   The samplerate is set relative to the base samplerate by a divider or to
    ///   a maximum samplerate.<br />
    ///   This divider is set by Tsr1Bits.samplerateId for values up to 5 with to
    ///   the following values:
    ///   <table>
    ///     <tr>
    ///       <td><b>Tsr1Bits.samplerateId</b></td><td>0</td><td>1</td><td>2</td><td>3</td>
    ///     </tr>
    ///     <tr>
    ///       <td><b>Samplerate</b></td><td>Max</td><td>Base</td><td>Base /
    ///       2</td><td>Base / 5</td>
    ///     </tr>
    ///   </table>
    ///   For higher divider values, the value can be set using the 16-bit value
    ///   in the two Downsampler bytes. The value of Downsampler is given by:<br
    ///   />
    ///   <i>Downsampler = 1comp((Base / Samplerate / 2) - 2)</i><br />
    ///   The Base samplerate is 50 MS/s for the DSO-2090 and DSO-2150. The Max
    ///   samplerate is also 50 MS/s for the DSO-2090 and 75 MS/s for the
    ///   DSO-2150.<br />
    ///   When using fast rate mode the Base and Max samplerate is twice as fast.
    ///   When Tsr1Bits.recordLength is 0 (Roll mode) the sampling rate is divided
    ///   by 1000.
    /// </p>
    /// <p>
    ///   The TriggerPosition sets the position of the pretrigger in samples. The
    ///   left side (0 %) is 0x77660 when using the small buffer and 0x78000 when
    ///   using the large buffer.
    /// </p>
    /// <p><br /></p>
    BULK_SETTRIGGERANDSAMPLERATE,

    /// BulkForceTrigger [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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

    /// BulkCaptureStart [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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

    /// BulkTriggerEnabled [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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

    /// BulkGetData [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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
    ///   The oscilloscope returns the sample data, that will be split if it's
    ///   larger than the IN endpoint packet length:
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
    ///   Because of the 10 bit data model, the DSO-5200 transmits the two extra
    ///   bits for each sample afterwards:
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

    /// BulkGetCaptureState [<em>::MODEL_DSO2090, ::MODEL_DSO2150,
    /// ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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
    ///   The oscilloscope returns it's capture state and the trigger point. Not
    ///   sure about this, looks like 248 16-bit words with nearly constant
    ///   values. These can be converted to the start address of the data in the
    ///   buffer (See Hantek::Control::calculateTriggerPoint):
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

    /// BulkSetGain [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
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
    ///   This command sets the logical data (Not used in official %Hantek
    ///   software):
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
    ///   This command reads the logical data (Not used in official %Hantek
    ///   software):
    ///   <table>
    ///     <tr>
    ///       <td>0x09</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The oscilloscope returns the logical data, which contains valid data in
    ///   the first byte although it is 64 or 512 bytes long:
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
    ///   The samplerate is set relative to the maximum sample rate by a divider
    ///   that is set in SamplerateFast and the 16-bit value in the two
    ///   SamplerateSlow bytes.<br />
    ///   Without using fast rate mode, the samplerate is:<br />
    ///   <i>Samplerate = SamplerateMax / (2comp(SamplerateSlow) * 2 + 4 -
    ///   SamplerateFast)</i><br />
    ///   SamplerateBase is 100 MS/s for the DSO-5200 in normal mode and 200 MS/s
    ///   in fast rate mode, the modifications regarding record length are the the
    ///   same that apply for the DSO-2090. The maximum samplerate is 125 MS/s in
    ///   normal mode and 250 MS/s in fast rate mode, and is reached by setting
    ///   SamplerateSlow = 0 and SamplerateFast = 4.
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
    ///   This command sets the trigger position and record length for the
    ///   DSO-5200:
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
    ///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger
    ///   position. Both values have a range from 0xd7ff (0xc7ff for 14 kiS
    ///   buffer) to 0xfffe. On the left side (0 %) the TriggerPositionPre value
    ///   is minimal, on the right side (100 %) it is maximal. The
    ///   TriggerPositionPost value is maximal for 0 % and minimal for 100%.
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
    ///   The downsampler can be activated by setting ESamplerateBits.downsampling
    ///   = 1. If this is the case, the value of Downsampler is given by:<br />
    ///   <i>Downsampler = 1comp((Base / Samplerate) - 2)</i><br />
    ///   Base is 100 MS/s for the DSO-2250 in standard mode and 200 MS/s in fast
    ///   rate mode, the modifications regarding record length are the the same
    ///   that apply for the DSO-2090. The maximum samplerate is 125 MS/s in
    ///   standard mode and 250 MS/s in fast rate mode and is achieved by setting
    ///   ESamplerateBits.downsampling = 0.
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
    ///   This command sets the trigger position and buffer configuration for the
    ///   DSO-2250:
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
    ///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger
    ///   position. Both values have a range from 0x7d800 (0x00000 for 512 kiS
    ///   buffer) to 0x7ffff. On the left side (0 %) the TriggerPositionPre value
    ///   is minimal, on the right side (100 %) it is maximal. The
    ///   TriggerPositionPost value is maximal for 0 % and minimal for 100%.
    /// </p>
    /// <p><br /></p>
    BULK_FSETBUFFER,

    BULK_COUNT
};

//////////////////////////////////////////////////////////////////////////////
/// \enum ControlCode                                           hantek/types.h
/// \brief All supported control commands.
enum ControlCode {
    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xa2 control read/write command gives access to a ::ControlValue.
    /// </p>
    /// <p><br /></p>
    CONTROL_VALUE = 0xa2,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xb2 control read command gets the speed level of the USB
    ///   connection:
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

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
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

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
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

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
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
    ///   The limits are <= instead of < for the 10 bit models, since those
    ///   support voltages up to 10 V.
    /// </p>
    /// <p><br /></p>
    CONTROL_SETRELAYS = 0xb5,

    CONTROL_SETVOLTDIV_CH1 = 0xe0,
    CONTROL_SETVOLTDIV_CH2 = 0xe1,
    CONTROL_SETTIMEDIV = 0xe2,
    CONTROL_ACQUIIRE_HARD_DATA = 0xe3
};

//////////////////////////////////////////////////////////////////////////////
/// \enum ControlValue                                          hantek/types.h
/// \brief All supported values for control commands.
enum ControlValue {
    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    /// Value 0x08 is the calibration data for the channels offsets. It holds the
    /// offset value for the top and bottom of the scope screen for every gain
    /// step on every channel. The data is stored as a three-dimensional array:<br
    /// />
    /// <i>channelLevels[channel][GainId][::LevelOffset]</i>
    /// </p>
    /// <p><br /></p>
    VALUE_OFFSETLIMITS = 0x08,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    /// Value 0x0a is the address of the device. It has a length of one byte.
    /// </p>
    /// <p><br /></p>
    VALUE_DEVICEADDRESS = 0x0a,

    /// <em>[::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
    /// <p>
    /// Value 0x60 is the calibration data for the fast rate mode on the DSO-2250,
    /// DSO-5200 and DSO-5200A. It's used to correct the level differences between
    /// the two merged channels to avoid deterministic noise.
    /// </p>
    /// <p><br /></p>
    VALUE_FASTRATECALIBRATION = 0x60,

    /// <em>[::MODEL_DSO5200, ::MODEL_DSO5200A]</em>
    /// <p>
    /// Value 0x70 contains correction values for the ETS functionality of the
    /// DSO-5200 and DSO-5200A.
    /// </p>
    /// <p><br /></p>
    VALUE_ETSCORRECTION = 0x70
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
enum TriggerSource { TRIGGER_CH2, TRIGGER_CH1, TRIGGER_ALT, TRIGGER_EXT, TRIGGER_EXT10 };

//////////////////////////////////////////////////////////////////////////////
/// \enum RecordLengthId                                        hantek/types.h
/// \brief The size id for CommandSetTriggerAndSamplerate.
enum RecordLengthId {
    RECORDLENGTHID_ROLL = 0, ///< Used for the roll mode
    RECORDLENGTHID_SMALL,    ///< The standard buffer with 10240 samples
    RECORDLENGTHID_LARGE     ///< The large buffer, 32768 samples (14336 for DSO-5200)
};

//////////////////////////////////////////////////////////////////////////////
/// \enum CaptureState                                          hantek/types.h
/// \brief The different capture states which the oscilloscope returns.
enum CaptureState {
    CAPTURE_WAITING = 0,   ///< The scope is waiting for a trigger event
    CAPTURE_SAMPLING = 1,  ///< The scope is sampling data after triggering
    CAPTURE_READY = 2,     ///< Sampling data is available (DSO-2090/DSO-2150)
    CAPTURE_READY2250 = 3, ///< Sampling data is available (DSO-2250)
    CAPTURE_READY5200 = 7, ///< Sampling data is available (DSO-5200/DSO-5200A)
    CAPTURE_ERROR = 1000
};

//////////////////////////////////////////////////////////////////////////////
/// \enum BulkIndex                                             hantek/types.h
/// \brief Can be set by CONTROL_BEGINCOMMAND, maybe it allows multiple commands
/// at the same time?
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
    OFFSET_END,   ///< The channel level at the top of the scope
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
    DTRIGGERPOSITION_ON = 7   ///< Used for normal operation
};

//////////////////////////////////////////////////////////////////////////////
/// \struct FilterBits                                          hantek/types.h
/// \brief The bits for BULK_SETFILTER.
struct FilterBits {
    uint8_t channel1 : 1; ///< Set to true when channel 1 isn't used
    uint8_t channel2 : 1; ///< Set to true when channel 2 isn't used
    uint8_t trigger : 1;  ///< Set to true when trigger isn't used
    uint8_t reserved : 5; ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct GainBits                                            hantek/types.h
/// \brief The gain bits for BULK_SETGAIN.
struct GainBits {
    uint8_t channel1 : 2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
    uint8_t channel2 : 2; ///< Gain for CH1, 0 = 1e* V, 1 = 2e*, 2 = 5e*
    uint8_t reserved : 4; ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct Tsr1Bits                                            hantek/types.h
/// \brief Trigger and samplerate bits (Byte 1).
struct Tsr1Bits {
    uint8_t triggerSource : 2;    ///< The trigger source, see Hantek::TriggerSource
    uint8_t recordLength : 3;     ///< See ::RecordLengthId
    uint8_t samplerateId : 2;     ///< Samplerate ID when downsampler is disabled
    uint8_t downsamplingMode : 1; ///< true, if Downsampler is used
};

//////////////////////////////////////////////////////////////////////////////
/// \struct Tsr2Bits                                            hantek/types.h
/// \brief Trigger and samplerate bits (Byte 2).
struct Tsr2Bits {
    uint8_t usedChannels : 2; ///< Used channels, see Hantek::UsedChannels
    uint8_t fastRate : 1;     ///< true, if one channels uses all buffers
    uint8_t triggerSlope : 1; ///< The trigger slope, see Dso::Slope, inverted
    /// when Tsr1Bits.samplerateFast is uneven
    uint8_t reserved : 4; ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct CTriggerBits                                        hantek/types.h
/// \brief Trigger bits for 0x0c command.
struct CTriggerBits {
    uint8_t triggerSource : 2; ///< The trigger source, see Hantek::TriggerSource
    uint8_t triggerSlope : 1;  ///< The trigger slope, see Dso::Slope
    uint8_t reserved : 5;      ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct DBufferBits                                         hantek/types.h
/// \brief Buffer mode bits for 0x0d command.
struct DBufferBits {
    uint8_t triggerPositionUsed : 3; ///< See ::DTriggerPositionUsed
    uint8_t recordLength : 3;        ///< See ::RecordLengthId
    uint8_t reserved : 2;            ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ESamplerateBits                                     hantek/types.h
/// \brief Samplerate bits for DSO-2250 0x0e command.
struct ESamplerateBits {
    uint8_t fastRate : 1;     ///< false, if one channels uses all buffers
    uint8_t downsampling : 1; ///< true, if the downsampler is activated
    uint8_t reserved : 4;     ///< Unused bits
};

//////////////////////////////////////////////////////////////////////////////
/// \struct ETsrBits                                            hantek/types.h
/// \brief Trigger and samplerate bits for DSO-5200/DSO-5200A 0x0e command.
struct ETsrBits {
    uint8_t fastRate : 1;      ///< false, if one channels uses all buffers
    uint8_t usedChannels : 2;  ///< Used channels, see Hantek::UsedChannels
    uint8_t triggerSource : 2; ///< The trigger source, see Hantek::TriggerSource
    uint8_t triggerSlope : 2;  ///< The trigger slope, see Dso::Slope
    uint8_t triggerPulse : 1;  ///< Pulses are causing trigger events
};
}
