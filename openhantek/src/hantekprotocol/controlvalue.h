#pragma once

namespace Hantek {


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

}

