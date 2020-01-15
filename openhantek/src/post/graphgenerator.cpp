// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>
#include <QMutex>
#include <exception>
#include <math.h>

#include "graphgenerator.h"
#include "ppresult.h"
#include "hantekdso/controlspecification.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "viewsettings.h"


static const SampleValues &useSpecSamplesOf(ChannelID channel, const PPresult *result,
                                            const DsoSettingsScope *scope) {
    static SampleValues emptyDefault;
    if (!scope->spectrum[channel].used || !result->data(channel)) return emptyDefault;
    return result->data(channel)->spectrum;
}


static const SampleValues &useVoltSamplesOf(ChannelID channel, const PPresult *result,
                                            const DsoSettingsScope *scope) {
    static SampleValues emptyDefault;
    if (!scope->voltage[channel].used || !result->data(channel)) return emptyDefault;
    return result->data(channel)->voltage;
}


GraphGenerator::GraphGenerator(const DsoSettingsScope *scope /*, const DsoSettingsView *view */) : scope(scope) /*, view(view) */{
    // printf( "GraphGenerator::GraphGenerator()\n" );
}


void GraphGenerator::process(PPresult *data) {
    //printf( "GraphGenerator::process()\n" );
    if (scope->horizontal.format == Dso::GraphFormat::TY) {
        ready = true;
        generateGraphsTYvoltage(data);
        generateGraphsTYspectrum(data);
    } else
        generateGraphsXY(data);
}


void GraphGenerator::generateGraphsTYvoltage(PPresult *result) {
    //printf( "GraphGenerator::generateGraphsTYvoltage()\n" );
    result->vaChannelVoltage.resize(scope->voltage.size());
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        ChannelGraph &target = result->vaChannelVoltage[channel];
        const SampleValues &samples = useVoltSamplesOf(channel, result, scope);

        // Check if this channel is used and available at the data analyzer
        if (samples.sample.empty()) {
            // Delete all vector arrays
            target.clear();
            continue;
        }

        // time distance between sampling points
        double horizontalFactor = (samples.interval / scope->horizontal.timebase);
        // printf( "hF: %g\n", horizontalFactor );
        unsigned dotsOnScreen = unsigned( ceil( DIVS_TIME / horizontalFactor ) );
        unsigned preTrigSamples = unsigned(scope->trigger.offset * dotsOnScreen);
        // align displayed trace with trigger mark on screen ...
        // ... also if trig pos or time/div was changed on a "frozen" or single trace
        int leftmostSample = int( result->triggeredPosition);
        if ( leftmostSample ) // adjust position if triggered, else start from sample[0]
            leftmostSample -= preTrigSamples; // shift samples to show a stable trace
        int leftmostPosition = 0; // start position on display
        if ( leftmostSample < 0 ) { // trig pos or time/div was increased
            leftmostPosition = -leftmostSample; // trace can't start on left margin
            leftmostSample = 0; // show as much as we have on left side
        }
        // Set size directly to avoid reallocations (n+1 dots to display n lines)
        target.reserve( ++dotsOnScreen );

        const double gain = scope->gain(channel);
        const double offset = scope->voltage[channel].offset;

        auto sampleIterator = samples.sample.cbegin() + leftmostSample; // -> visible samples
        auto sampleEnd = samples.sample.cend();
        // printf("samples: %lu, dotsOnScreen: %d\n", samples.sample.size(), dotsOnScreen);
        target.clear(); // remove all previous dots and fill in new trace
        for (unsigned int position = unsigned(leftmostPosition);
             position < dotsOnScreen && sampleIterator < sampleEnd;
             ++position, ++sampleIterator) {
            target.push_back(QVector3D(float(MARGIN_LEFT + position * horizontalFactor),
                                        float(*sampleIterator / gain + offset), 0.0f ));
        }
    }
}


void GraphGenerator::generateGraphsTYspectrum(PPresult *result) {
    //printf( "GraphGenerator::generateGraphsTYspectrum()\n" );
    ready = true;
    result->vaChannelSpectrum.resize(scope->spectrum.size());
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        ChannelGraph &target = result->vaChannelSpectrum[channel];
        const SampleValues &samples = useSpecSamplesOf(channel, result, scope);

        // Check if this channel is used and available at the data analyzer
        if (samples.sample.empty()) {
            // Delete all vector arrays
            target.clear();
            continue;
        }
        // Check if the sample count has changed
        size_t sampleCount = samples.sample.size();
        size_t neededSize = sampleCount * 2;

        // Set size directly to avoid reallocations
        target.reserve(neededSize);

        // What's the horizontal distance between sampling points?
        double horizontalFactor = samples.interval / scope->horizontal.frequencybase;

        // Fill vector array
        std::vector<double>::const_iterator dataIterator = samples.sample.begin();
        const double magnitude = scope->spectrum[channel].magnitude;
        const double offset = scope->spectrum[channel].offset;

        for (unsigned int position = 0; position < sampleCount; ++position) {
            target.push_back( QVector3D( float( position * horizontalFactor - DIVS_TIME / 2 ),
                                         float( *(dataIterator++) / magnitude + offset), 0.0f ) );
        }
    }
}


void GraphGenerator::generateGraphsXY(PPresult *result) {
    result->vaChannelVoltage.resize( scope->voltage.size() );

    // Delete all spectrum graphs
    for ( ChannelGraph &data : result->vaChannelSpectrum ) data.clear();

    // Generate voltage graphs for pairs of channels
    for ( ChannelID channel = 0; channel < scope->voltage.size(); channel += 2 ) {
        // We need pairs of channels.
        if ( channel + 1 == scope->voltage.size() ) {
            result->vaChannelVoltage[ channel ].clear();
            continue;
        }

        const ChannelID xChannel = channel;
        const ChannelID yChannel = channel + 1;

        const SampleValues &xSamples = useVoltSamplesOf( xChannel, result, scope );
        const SampleValues &ySamples = useVoltSamplesOf( yChannel, result, scope );

        // The channels need to be active
        if ( !xSamples.sample.size() || !ySamples.sample.size() ) {
            result->vaChannelVoltage[ xChannel ].clear();
            result->vaChannelVoltage[ yChannel ].clear();
            continue;
        }

        // Check if the sample count has changed
        const size_t sampleCount = std::min( xSamples.sample.size(), ySamples.sample.size() );
        ChannelGraph &drawLines = result->vaChannelVoltage[ yChannel ]; // color of y channel
        drawLines.reserve( sampleCount * 2 );

        // Fill vector array
        std::vector<double>::const_iterator xIterator = xSamples.sample.begin();
        std::vector<double>::const_iterator yIterator = ySamples.sample.begin();
        const double xGain = scope->gain( xChannel );
        const double yGain = scope->gain( yChannel );
        const double xOffset = ( scope->trigger.offset - 0.5 ) * DIVS_TIME;
        const double yOffset = scope->voltage[ yChannel ].offset;

        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            drawLines.push_back(QVector3D( float( *( xIterator++ ) / xGain + xOffset ),
                                           float( *( yIterator++ ) / yGain + yOffset ), 0.0 ) );
        }
    }
}
