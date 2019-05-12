#include "triggering.h"
#include "scopesettings.h"
#include "post/postprocessingsettings.h"
#include "post/softwaretrigger.h"
#include "enums.h"

Triggering::Triggering(const DsoSettingsScope *scope, bool isSoftwareTriggerDevice)
    : scope(scope), isSoftwareTriggerDevice(isSoftwareTriggerDevice) {}

Triggering::~Triggering() {}


void Triggering::process(PPresult *result) {
    // printf( "Triggering::process()\n" );
    static std::vector<SampleValues> lastSamples( 3 );     // resize later if needed
    static std::vector<unsigned> lastSkip( 8, 0 );         // reserve storage for 8 channels
    static std::vector<bool> samplesTriggered( 8, false ); // samples from triggered trace
    static std::vector<bool> samplesValid( 8, false );     // all samples are invalid (clipped etc.)

    // check trigger point for software trigger
    if (isSoftwareTriggerDevice && scope->trigger.source < result->channelCount())
        std::tie( result->softwareTriggerTriggered, result->skipSamples ) = SoftwareTrigger::compute(result, scope);

    result->vaChannelVoltage.resize(scope->voltage.size());

    for (ChannelID channel = 0; channel < result->channelCount(); ++channel) {
        if ( result->softwareTriggerTriggered ) {
            // If we have a triggered trace then save and display this trace
            if ( lastSamples.size() <= channel ) // increase vector size if needed
                lastSamples.resize( channel+1 );
            lastSamples[ channel ] = result->data(channel)->voltage;
            lastSkip[ channel ] = result->skipSamples;
            samplesTriggered[ channel ] = true;
            samplesValid[ channel ] = result->data(channel)->valid;
        } else if ( scope->trigger.mode == Dso::TriggerMode::NORMAL && samplesTriggered[ channel ] ) {
            // If not triggered in NORMAL mode but a triggered trace was saved then use the saved trace
            result->skipSamples = lastSkip[ channel ]; // return last triggered value
            result->modifyData(channel)->voltage = lastSamples[ channel ];
            result->modifyData(channel)->valid = samplesValid[ channel ];
        } else {
            // If not triggered, but not NORMAL mode -> discard history and use the free running trace
            samplesTriggered[ channel ] = false;
        }
    }
}
