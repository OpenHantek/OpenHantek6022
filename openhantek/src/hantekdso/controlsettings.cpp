#include "controlsettings.h"
#include "definitions.h"

namespace Hantek {

ControlSettings::ControlSettings(ControlSamplerateLimits* limits)
{
    samplerate.limits = limits;
    trigger.level.resize(HANTEK_CHANNELS);
    voltage.resize(HANTEK_CHANNELS);
    for (unsigned channel = 0; channel < HANTEK_CHANNELS; ++channel) {
        trigger.level[channel] = 0.0;
        voltage[channel].gain = 0;
        voltage[channel].offset = 0.0;
        voltage[channel].offsetReal = 0.0;
        voltage[channel].used = false;
    }
}

}
