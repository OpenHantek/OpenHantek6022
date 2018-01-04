#include "controlsettings.h"

namespace Dso {

ControlSettings::ControlSettings(ControlSamplerateLimits* limits, size_t channelCount)
{
    samplerate.limits = limits;
    trigger.level.resize(channelCount);
    voltage.resize(channelCount);
    for (ChannelID channel = 0; channel < channelCount; ++channel) {
        trigger.level[channel] = 0.0;
        voltage[channel].gain = 0;
        voltage[channel].offset = 0.0;
        voltage[channel].offsetReal = 0.0;
        voltage[channel].used = false;
    }
}

}
