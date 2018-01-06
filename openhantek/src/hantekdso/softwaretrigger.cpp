#include "softwaretrigger.h"
#include "analyse/dataanalyzerresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "utils/printutils.h"

SoftwareTrigger::PrePostStartTriggerSamples SoftwareTrigger::compute(const DataAnalyzerResult *data,
                                                                              const DsoSettingsScope *scope)
{
    unsigned int preTrigSamples = 0;
    unsigned int postTrigSamples = 0;
    unsigned int swTriggerStart = 0;
    ChannelID channel = scope->trigger.source;

    // Trigger channel not in use
    if (!scope->voltage[channel].used || !data->data(channel) ||
            data->data(channel)->voltage.sample.empty())
        return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);

    const std::vector<double>& samples = data->data(channel)->voltage.sample;
    double level = scope->voltage[channel].trigger;
    size_t sampleCount = samples.size();
    double timeDisplay = scope->horizontal.timebase * DIVS_TIME;
    double samplesDisplay = timeDisplay * scope->horizontal.samplerate;

    if (samplesDisplay >= sampleCount) {
        // For sure not enough samples to adjust for jitter.
        // Following options exist:
        //    1: Decrease sample rate
        //    2: Change trigger mode to auto
        //    3: Ignore samples
        // For now #3 is chosen
        timestampDebug(QString("Too few samples to make a steady "
                               "picture. Decrease sample rate"));
        return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);
    }
    preTrigSamples = (unsigned)(scope->trigger.position * samplesDisplay);
    postTrigSamples = (unsigned)sampleCount - ((unsigned)samplesDisplay - preTrigSamples);

    double prev;
    bool (*opcmp)(double,double,double);
    bool (*smplcmp)(double,double);
    if (scope->trigger.slope == Dso::Slope::Positive) {
        prev = INT_MAX;
        opcmp = [](double value, double level, double prev) { return value > level && prev <= level;};
        smplcmp = [](double sampleK, double value) { return sampleK >= value;};
    } else {
        prev = INT_MIN;
        opcmp = [](double value, double level, double prev) { return value < level && prev >= level;};
        smplcmp = [](double sampleK, double value) { return sampleK < value;};
    }

    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
        double value = samples[i];
        if (opcmp(value, level, prev)) {
            unsigned rising = 0;
            for (unsigned int k = i + 1; k < i + scope->trigger.swTriggerSampleSet && k < sampleCount; k++) {
                if (smplcmp(samples[k], value)) { rising++; }
            }
            if (rising > scope->trigger.swTriggerThreshold) {
                swTriggerStart = i;
                break;
            }
        }
        prev = value;
    }
    if (swTriggerStart == 0) {
        timestampDebug(QString("Trigger not asserted. Data ignored"));
        preTrigSamples = 0; // preTrigSamples may never be greater than swTriggerStart
        postTrigSamples = 0;
    }
    return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);
}
