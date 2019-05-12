#include "post/softwaretrigger.h"
#include "post/ppresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "utils/printutils.h"

SoftwareTrigger::TriggerStatusSkip SoftwareTrigger::compute(const PPresult *data, const DsoSettingsScope *scope)
{
    // printf( "SoftwareTrigger::compute()\n" );
    unsigned int preTrigSamples = 0;
    unsigned int postTrigSamples = 0;
    unsigned int swTriggerStart = 0;
    ChannelID channel = scope->trigger.source;

    // Trigger channel not in use
    if (!scope->voltage[channel].used || !data->data(channel) ||
            data->data(channel)->voltage.sample.empty())
        return TriggerStatusSkip( false, 0 );

    const std::vector<double>& samples = data->data(channel)->voltage.sample;
    double level = scope->voltage[channel].trigger;
    size_t sampleCount = samples.size(); // number of available samples
    double timeDisplay = scope->horizontal.timebase * DIVS_TIME; // time for full screen width
    double samplesDisplay = timeDisplay * scope->horizontal.samplerate; // samples for full screen width
    if (samplesDisplay >= sampleCount) { 
        // For sure not enough samples to adjust for jitter.
        // Following options exist:
        //    1: Decrease sample rate
        //    2: Change trigger mode to auto
        //    3: Ignore samples
        // For now #3 is chosen
        timestampDebug(QString("Too few samples to make a steady "
                               "picture. Decrease sample rate"));
        return TriggerStatusSkip( false, 0 );
    }
    preTrigSamples = (unsigned)(scope->trigger.position * samplesDisplay); // samples left of trigger
    postTrigSamples = (unsigned)sampleCount - ((unsigned)samplesDisplay - preTrigSamples); // samples right of trigger
    // I-----------samples-----------I
    // I--disp--I
    // I-----T--I--------------------I
    // I-pre-I
    // I--(samp-disp+pre)--------I
    double prev;
    bool (*opcmp)(double,double,double);
    bool (*smplcmpBefore)(double,double);
    bool (*smplcmpAfter)(double,double);
    // define trigger condition
    if (scope->trigger.slope == Dso::Slope::Positive) {
        prev = INT_MAX;
        opcmp = [](double value, double level, double prev) { return value > level && prev <= level;};
        smplcmpBefore = [](double sampleK, double value) { return sampleK < value;};
        smplcmpAfter = [](double sampleK, double value) { return sampleK >= value;};
    } else {
        prev = INT_MIN;
        opcmp = [](double value, double level, double prev) { return value < level && prev >= level;};
        smplcmpBefore = [](double sampleK, double value) { return sampleK >= value;};
        smplcmpAfter = [](double sampleK, double value) { return sampleK < value;};
    }
    // search for trigger point in a range that leaves enough samples left and right of trigger for display
    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
        double value = samples[i];
        if (opcmp(value, level, prev)) { // trigger condition met
            // check for the next few SampleSet samples, if they are also above/below the trigger value
            // defined in src/scopesettings.h
            unsigned int risingBefore = 0;
            for (unsigned int k = i - 1; k > i - scope->trigger.swTriggerSampleSet && k > 0; k--) {
                if (smplcmpBefore(samples[k], level)) { risingBefore++; }
            }
            unsigned int risingAfter = 0;
            for (unsigned int k = i + 1; k < i + scope->trigger.swTriggerSampleSet && k < sampleCount; k++) {
                if (smplcmpAfter(samples[k], level)) { risingAfter++; }
            }

            // if at least >Threshold (=5) samples before and after trig meet the condition, set trigger
            if (risingBefore > scope->trigger.swTriggerThreshold && risingAfter > scope->trigger.swTriggerThreshold) {
                swTriggerStart = i-1;
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
    //printf("PPS(%d %d %d)\n", preTrigSamples, postTrigSamples, swTriggerStart);
    return TriggerStatusSkip( postTrigSamples > preTrigSamples, swTriggerStart - preTrigSamples );
}
