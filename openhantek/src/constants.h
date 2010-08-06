////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file constants.h
/// \brief Defines global constants and enums.
//
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


#ifndef CONSTANTS_H
#define CONSTANTS_H


#define MARKER_COUNT                  2 ///< Number of markers
//#define PHYS_CHANNEL_COUNT            2 ///< Number of real channels


////////////////////////////////////////////////////////////////////////////////
/// \namespace Dso                                                   constants.h
/// \brief All DSO specific enums for different modes and so on.
namespace Dso {
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ChannelMode                                              constants.h
	/// \brief The channel display modes.
	enum ChannelMode {
		CHANNELMODE_VOLTAGE,                ///< Standard voltage view
		CHANNELMODE_SPECTRUM,               ///< Spectrum view
		CHANNELMODE_COUNT                   ///< The total number of modes
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum GraphFormat                                              constants.h
	/// \brief The possible viewing formats for the graphs on the scope.
	enum GraphFormat {
		GRAPHFORMAT_TY,                     ///< The standard mode
		GRAPHFORMAT_XY                      ///< CH1 on X-axis, CH2 on Y-axis
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum Coupling                                                 constants.h
	/// \brief The coupling modes for the channels.
	enum Coupling {
		COUPLING_AC,                        ///< Offset filtered out by condensator
		COUPLING_DC,                        ///< No filtering
		COUPLING_GND                        ///< Channel is grounded
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum Slope                                                    constants.h
	/// \brief The slope that causes a trigger.
	enum Slope {
		SLOPE_POSITIVE,                     ///< From lower to higher voltage
		SLOPE_NEGATIVE                      ///< From higher to lower voltage
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum TriggerMode                                              constants.h
	/// \brief The different triggering modes.
	enum TriggerMode {
		TRIGGERMODE_AUTO,                   ///< Automatic without trigger event
		TRIGGERMODE_NORMAL,                 ///< Normal mode
		TRIGGERMODE_SINGLE                  ///< Stop after the first trigger event
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum MathMode                                                 constants.h
	/// \brief The different math modes for the math-channel.
	enum MathMode {
		MATHMODE_1ADD2,                     ///< Add the values of the channels
		MATHMODE_1SUB2,                     ///< Subtract CH2 from CH1
		MATHMODE_2SUB1                      ///< Subtract CH1 from CH2
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum WindowFunction                                           constants.h
	/// \brief The supported window functions.
	/// These are needed for spectrum analysis and are applied to the sample values
	/// before calculating the DFT.
	enum WindowFunction {
		WINDOW_RECTANGULAR,                 ///< Rectangular window (aka Dirichlet)
		WINDOW_HAMMING,                     ///< Hamming window
		WINDOW_HANN,                        ///< Hann window
		WINDOW_COSINE,                      ///< Cosine window (aka Sine)
		WINDOW_LANCZOS,                     ///< Lanczos window (aka Sinc)
		WINDOW_BARTLETT,                    ///< Bartlett window (Endpoints == 0)
		WINDOW_TRIANGULAR,                  ///< Triangular window (Endpoints != 0)
		WINDOW_GAUSS,                       ///< Gauss window (simga = 0.4)
		WINDOW_BARTLETTHANN,                ///< Bartlett-Hann window
		WINDOW_BLACKMAN,                    ///< Blackman window (alpha = 0.16)
		//WINDOW_KAISER,                      ///< Kaiser window (alpha = 3.0)
		WINDOW_NUTTALL,                     ///< Nuttall window, cont. first deriv.
		WINDOW_BLACKMANHARRIS,              ///< Blackman-Harris window
		WINDOW_BLACKMANNUTTALL,             ///< Blackman-Nuttall window
		WINDOW_FLATTOP                      ///< Flat top window
	};
}

////////////////////////////////////////////////////////////////////////////////
/// \enum GlInterpolationMode                                        constants.h
/// \brief The different interpolation modes for the graphs.
enum GlInterpolationMode {
	INTERPOLATION_OFF = 0,                ///< Just dots for each sample
	INTERPOLATION_LINEAR,                 ///< Sample dots connected by lines
	INTERPOLATION_SINC                    ///< Smooth graph through the dots
};


#endif
