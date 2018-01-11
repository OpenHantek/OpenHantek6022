#include "mathchannelgenerator.h"
#include "scopesettings.h"

MathChannelGenerator::MathChannelGenerator(const DsoSettingsScope *scope, unsigned physicalChannels)
    : physicalChannels(physicalChannels), scope(scope) {}

MathChannelGenerator::~MathChannelGenerator() {}

void MathChannelGenerator::process(PPresult *result) {
    bool channelsHaveData = !result->data(0)->voltage.sample.empty() && !result->data(1)->voltage.sample.empty();
    if (!channelsHaveData) return;

    for (ChannelID channel = physicalChannels; channel < result->channelCount(); ++channel) {
        DataChannel *const channelData = result->modifyData(channel);

        // Math channel enabled?
        if (!scope->voltage[channel].used && !scope->spectrum[channel].used) continue;

        // Set sampling interval
        channelData->voltage.interval = result->data(0)->voltage.interval;

        // Resize the sample vector
        std::vector<double> &resultData = channelData->voltage.sample;
        resultData.resize(std::min(result->data(0)->voltage.sample.size(), result->data(1)->voltage.sample.size()));

        // Calculate values and write them into the sample buffer
        std::vector<double>::const_iterator ch1Iterator = result->data(0)->voltage.sample.begin();
        std::vector<double>::const_iterator ch2Iterator = result->data(1)->voltage.sample.begin();
        for (std::vector<double>::iterator it = resultData.begin(); it != resultData.end(); ++it) {
            switch (scope->voltage[physicalChannels].math) {
            case Dso::MathMode::ADD_CH1_CH2:
                *it = *ch1Iterator + *ch2Iterator;
                break;
            case Dso::MathMode::SUB_CH2_FROM_CH1:
                *it = *ch1Iterator - *ch2Iterator;
                break;
            case Dso::MathMode::SUB_CH1_FROM_CH2:
                *it = *ch2Iterator - *ch1Iterator;
                break;
            }
            ++ch1Iterator;
            ++ch2Iterator;
        }
    }
}
