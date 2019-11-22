// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>
#include <QMutex>
#include <exception>
#include <math.h>

#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "hantekdso/controlspecification.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "viewsettings.h"

// M_PI is not mandatory in math.h / cmath
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

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


GraphGenerator::GraphGenerator(const DsoSettingsScope *scope, const DsoSettingsView *view) : scope(scope), view(view) {
    // printf( "GraphGenerator::GraphGenerator()\n" );
    prepareSinc();
}


void GraphGenerator::prepareSinc( void ) {
    // prepare a sinc table (without sinc(0))
    sinc.clear();
    sinc.resize( sincSize );
    auto sincIt = sinc.begin();
    for ( unsigned int pos = 1; pos <= sincSize; ++pos, ++sincIt ) {
        double t = pos * M_PI / oversample;
        // Hann window: 0.5 + 0.5 cos, Hamming: 0.54 + 0.46 cos
        double w = 0.54 + 0.46 * cos( pos * M_PI / sincSize );
        *sincIt = w * sin( t ) / t;
    }
}


void GraphGenerator::process(PPresult *data) {
    //printf( "GraphGenerator::process()\n" );
    if (scope->horizontal.format == Dso::GraphFormat::TY) {
        ready = true;
        generateGraphsTYvoltage(data, view);
        generateGraphsTYspectrum(data);
    } else
        generateGraphsXY(data, scope);
}


void GraphGenerator::generateGraphsTYvoltage(PPresult *result, const DsoSettingsView *view) {
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
        float horizontalFactor = (float)(samples.interval / scope->horizontal.timebase);
        // printf( "hF: %g\n", horizontalFactor );
        unsigned dotsOnScreen = DIVS_TIME / horizontalFactor + 0.99; // round up
        unsigned preTrigSamples = (unsigned)(scope->trigger.position * dotsOnScreen);
        // align displayed trace with trigger mark on screen ...
        // ... also if trig pos or time/div was changed on a "frozen" or single trace
        int leftmostSample = result->triggerPosition - preTrigSamples; // 1st sample to show
        int leftmostPosition = 0; // start position on display
        if ( leftmostSample < 0 ) { // trig pos or time/div was increased
            leftmostPosition = -leftmostSample; // trace can't start on left margin
            leftmostSample = 0; // show as much as we have on left side
        }
        // Set size directly to avoid reallocations (n+1 dots to display n lines)
        target.reserve( ++dotsOnScreen );

        const float gain = (float)scope->gain(channel);
        const float offset = (float)scope->voltage[channel].offset;

        auto sampleIterator = samples.sample.cbegin() + leftmostSample; // -> visible samples
        auto sampleEnd = samples.sample.cend();
        // sinc interpolation in case of too less samples on screen
        // https://ccrma.stanford.edu/~jos/resample/resample.pdf
        if ( view->interpolation == Dso::INTERPOLATION_SINC
            && dotsOnScreen < 100 ) { // valid for timebase <= 500 ns/div
            const unsigned int sincSize = sinc.size();
            // if untriggered (skipSamples == 0) then reserve margin for left side of sinc()
            const unsigned int skip = leftmostSample ? leftmostSample : sincWidth;
            // we would need sincWidth on left side, but we take what we get
            const unsigned int left = std::min( sincWidth, skip );
            const unsigned int resampleSize = (left + dotsOnScreen + sincWidth) * oversample;
            std::vector <double> resample;
            resample.resize( resampleSize ); // prefilled with zero
            horizontalFactor /= oversample; // distance between (resampled) dots
            dotsOnScreen = DIVS_TIME / horizontalFactor + 0.99 + 1; // dot count after resample
            target.reserve( dotsOnScreen ); // increase target size
            // sampleIt -> start of left margin
            auto sampleIt = samples.sample.cbegin() + skip - left;
            for ( unsigned int resamplePos = 0; resamplePos < resampleSize; resamplePos += oversample, ++sampleIt ) {
                resample[ resamplePos ] += *sampleIt; //  * sinc( 0 )
                auto sincIt = sinc.cbegin(); // one half of sinc pulse without sinc(0) 
                for ( unsigned int sincPos = 1; sincPos <= sincSize; ++sincPos, ++sincIt ) { // sinc( 1..n )
                    const double conv = *sampleIt * *sincIt;
                    if ( resamplePos >= sincPos ) // left half of sinc in visible range
                        resample[ resamplePos - sincPos ] += conv;
                    if ( resamplePos + sincPos < resampleSize ) // right half of sinc visible
                        resample[ resamplePos + sincPos ] += conv;
                }
            }
            sampleIterator = resample.cbegin() + ( left + 0.5 ) * oversample; // -> visible resamples
        }
        // printf("samples: %lu, dotsOnScreen: %d\n", samples.sample.size(), dotsOnScreen);
        target.clear(); // remove all previous dots and fill in new trace
        for (unsigned int position = leftmostPosition;
             position < dotsOnScreen && sampleIterator < sampleEnd;
             ++position, ++sampleIterator) {
            target.push_back(QVector3D(MARGIN_LEFT + position * horizontalFactor,
                                        *sampleIterator / gain + offset, 0.0 ));
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
        float horizontalFactor = (float)(samples.interval / scope->horizontal.frequencybase);

        // Fill vector array
        std::vector<double>::const_iterator dataIterator = samples.sample.begin();
        const float magnitude = (float)scope->spectrum[channel].magnitude;
        const float offset = (float)scope->spectrum[channel].offset;

        for (unsigned int position = 0; position < sampleCount; ++position) {
            target.push_back(QVector3D(position * horizontalFactor - DIVS_TIME / 2,
                                       (float)*(dataIterator++) / magnitude + offset, 0.0));
        }
    }
}


void GraphGenerator::generateGraphsXY(PPresult *result, const DsoSettingsScope *scope) {
    result->vaChannelVoltage.resize(scope->voltage.size());

    // Delete all spectrum graphs
    for (ChannelGraph &data : result->vaChannelSpectrum) data.clear();

    // Generate voltage graphs for pairs of channels
    for (ChannelID channel = 0; channel < scope->voltage.size(); channel += 2) {
        // We need pairs of channels.
        if (channel + 1 == scope->voltage.size()) {
            result->vaChannelVoltage[channel].clear();
            continue;
        }

        const ChannelID xChannel = channel;
        const ChannelID yChannel = channel + 1;

        const SampleValues &xSamples = useVoltSamplesOf(xChannel, result, scope);
        const SampleValues &ySamples = useVoltSamplesOf(yChannel, result, scope);

        // The channels need to be active
        if (!xSamples.sample.size() || !ySamples.sample.size()) {
            result->vaChannelVoltage[channel].clear();
            result->vaChannelVoltage[channel + 1].clear();
            continue;
        }

        // Check if the sample count has changed
        const size_t sampleCount = std::min(xSamples.sample.size(), ySamples.sample.size());
        ChannelGraph &drawLines = result->vaChannelVoltage[channel];
        drawLines.reserve(sampleCount * 2);

        // Fill vector array
        std::vector<double>::const_iterator xIterator = xSamples.sample.begin();
        std::vector<double>::const_iterator yIterator = ySamples.sample.begin();
        const double xGain = scope->gain(xChannel);
        const double yGain = scope->gain(yChannel);
        const double xOffset = scope->voltage[xChannel].offset;
        const double yOffset = scope->voltage[yChannel].offset;

        for (unsigned int position = 0; position < sampleCount; ++position) {
            drawLines.push_back(QVector3D((float)(*(xIterator++) / xGain + xOffset),
                                          (float)(*(yIterator++) / yGain + yOffset), 0.0));
        }
    }
}
