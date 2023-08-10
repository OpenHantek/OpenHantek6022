// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDebug>
#include <QMutex>
#include <math.h>

#include "graphgenerator.h"
#include "hantekdso/controlspecification.h"
#include "ppresult.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "viewsettings.h"


static const SampleValues &useSpecSamplesOf( ChannelID channel, const PPresult *result, const DsoSettingsScope *scope ) {
    static SampleValues emptyDefault;
    if ( !scope->spectrum[ channel ].used || !result->data( channel ) )
        return emptyDefault;
    return result->data( channel )->spectrum;
}


static const SampleValues &useVoltSamplesOf( ChannelID channel, const PPresult *result, const DsoSettingsScope *scope ) {
    static SampleValues emptyDefault;
    if ( !scope->voltage[ channel ].used || !result->data( channel ) )
        return emptyDefault;
    return result->data( channel )->voltage;
}


GraphGenerator::GraphGenerator( const DsoSettingsScope *scope, const DsoSettingsView *view ) : scope( scope ), view( view ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " GraphGenerator::GraphGenerator()";
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


void GraphGenerator::process( PPresult *result ) {
    if ( scope->verboseLevel > 4 )
        qDebug() << "    GraphGenerator::process()" << result->tag;
    if ( scope->horizontal.format == Dso::GraphFormat::TY ) {
        ready = true;
        generateGraphsTYvoltage( result );
        generateGraphsTYspectrum( result );
    } else
        generateGraphsXY( result );
}


void GraphGenerator::generateGraphsTYvoltage( PPresult *result ) {
    if ( scope->verboseLevel > 5 )
        qDebug() << "     GraphGenerator::generateGraphsTYvoltage()" << result->tag;
    result->vaChannelVoltage.resize( scope->voltage.size() );
    result->vaChannelHistogram.resize( scope->voltage.size() );
    bool interpolationStep = view->interpolation == Dso::INTERPOLATION_STEP;
    bool interpolationSinc = view->interpolation == Dso::INTERPOLATION_SINC;
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelGraph &graphVoltage = result->vaChannelVoltage[ channel ];
        ChannelGraph &graphHistogram = result->vaChannelHistogram[ channel ];
        const SampleValues &sampleValues = useVoltSamplesOf( channel, result, scope );

        // Check if this channel is used and available at the data analyzer
        if ( sampleValues.samples.empty() ) {
            // Delete all vector arrays
            graphVoltage.clear();
            graphHistogram.clear();
            continue;
        }

        // time distance between sampling points
        double horizontalFactor = ( sampleValues.interval / scope->horizontal.timebase );
        // printf( "hF: %g\n", horizontalFactor );
        unsigned dotsOnScreen = unsigned( ceil( DIVS_TIME / horizontalFactor ) );
        unsigned preTrigSamples = unsigned( scope->trigger.position * dotsOnScreen );
        // align displayed trace with trigger mark on screen ...
        // ... also if trig pos or time/div was changed on a "frozen" or single trace
        int leftmostSample = int( result->triggeredPosition );
        if ( leftmostSample )                     // adjust position if triggered, else start from sample[0]
            leftmostSample -= preTrigSamples + 1; // shift samples to show a stable trace
        int leftmostPosition = 0;                 // start position on display
        if ( leftmostSample < 0 ) {               // trig pos or time/div was increased
            leftmostPosition = -leftmostSample;   // trace can't start on left margin
            leftmostSample = 0;                   // show as much as we have on left side
        }

        const unsigned binsPerDiv = 50; // resolution of histogram

        // Set size directly to avoid reallocations (n+1 dots to display n lines)
        graphVoltage.reserve( ++dotsOnScreen * ( interpolationStep ? 2 : 1 ) ); // two dots per "Step"
        graphHistogram.reserve( int( 2 * ( binsPerDiv * DIVS_VOLTAGE ) ) );

        const double gain = scope->gain( channel );
        const double offset = scope->voltage[ channel ].offset;

        auto sampleIterator = sampleValues.samples.cbegin() + leftmostSample; // -> visible samples
        auto sampleEnd = sampleValues.samples.cend();

        // sinc interpolation if there are too less samples on screen
        // https://ccrma.stanford.edu/~jos/resample/resample.pdf
        if ( interpolationSinc && dotsOnScreen < view->screenWidth ) {
            // we would need sincWidth, but we take what we get
            const unsigned int left = std::min( sincWidth, unsigned( leftmostSample ) );
            horizontalFactor /= oversample;                                     // distance between (resampled) dots
            dotsOnScreen = unsigned( DIVS_TIME / horizontalFactor + 0.99 + 1 ); // dot count after resample
            const unsigned int resampleSize = ( left + dotsOnScreen + sincWidth ) * oversample;
            resample.clear();                // invalidate old content
            resample.resize( resampleSize ); //  ... and init with zero because we accumulate the convolution
            auto sampleIt = sampleValues.samples.cbegin() + leftmostSample;
            for ( unsigned int resamplePos = 0; resamplePos < resampleSize; resamplePos += oversample ) {
                resample[ resamplePos ] += *sampleIt; // sinc( 0 ) sum up, do NOT assign
                auto sincIt = sinc.cbegin();          // -> one half of sinc pulse without sinc(0)
                for ( unsigned int sincPos = 1; sincPos <= sincSize; ++sincPos ) {
                    const double convolute = *sampleIt * *sincIt;
                    if ( resamplePos >= sincPos ) // left half of sinc in visible range
                        resample[ resamplePos - sincPos ] += convolute;
                    if ( resamplePos + sincPos < resampleSize ) // right half of sinc visible
                        resample[ resamplePos + sincPos ] += convolute;
                    ++sincIt;
                }
                ++sampleIt;
            }
            leftmostPosition *= oversample;            // scale the position accordingly
            graphVoltage.reserve( resampleSize );      // provide enough space for resampled dots
            sampleIterator = resample.cbegin() + left; // now switch from samples -> resamples
            sampleEnd = resample.cend();               // ... same for end of samples
        }

        graphVoltage.clear();   // remove all previous dots and fill in new trace as GL_LINE_STRIP
        graphHistogram.clear(); // remove all previous line and fill in new histo as GL_LINES
        unsigned bins[ int( binsPerDiv * DIVS_VOLTAGE ) ] = { 0 };
        for ( unsigned int position = unsigned( leftmostPosition ); position < dotsOnScreen && sampleIterator < sampleEnd - 1;
              ++position ) {
            double x = double( MARGIN_LEFT + position * horizontalFactor );
            double y_1 = *sampleIterator++ / gain + offset;
            double y = *sampleIterator / gain + offset;
            if ( !scope->histogram ) { // show complete trace
                if ( interpolationStep )
                    graphVoltage.push_back( QVector3D( float( x ), float( y_1 ), 0.0f ) ); // insert horizontal step
                graphVoltage.push_back( QVector3D( float( x ), float( y ), 0.0f ) );
            } else { // histogram replaces trace in rightmost div
                int bin = int( round( binsPerDiv * ( y + DIVS_VOLTAGE / 2 ) ) );
                if ( bin > 0 && bin < binsPerDiv * DIVS_VOLTAGE ) // count value if trace is on screen
                    ++bins[ bin ];
                if ( x < MARGIN_RIGHT - 1.1 ) { // show trace unless in last div + 10% margin
                    if ( interpolationStep )
                        graphVoltage.push_back( QVector3D( float( x ), float( y_1 ), 0.0f ) ); // horizontal step
                    graphVoltage.push_back( QVector3D( float( x ), float( y ), 0.0f ) );
                }
            }
        }

        if ( ( scope->horizontal.format == Dso::GraphFormat::TY ) && scope->histogram ) { // scale and display the histogram
            double max = 0;                                                               // find max histo count
            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                if ( bins[ bin ] > max ) {
                    max = bins[ bin ];
                }
            }
            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                if ( bins[ bin ] ) { // show bar (= start and end point) if value exists
                    double y = double( bin ) / binsPerDiv - DIVS_VOLTAGE / 2 - double( channel ) / binsPerDiv / 2;
                    // draw a line (as GL_LINES) with from MARGIN_RIGHT to the normalised histo size of this bin
                    graphHistogram.push_back( QVector3D( float( MARGIN_RIGHT ), float( y ), 0 ) );
                    graphHistogram.push_back( QVector3D( float( MARGIN_RIGHT - bins[ bin ] / max ), float( y ), 0 ) );
                }
            }
        }
    }
}


void GraphGenerator::generateGraphsTYspectrum( PPresult *result ) {
    if ( scope->verboseLevel > 5 )
        qDebug() << "     GraphGenerator::generateGraphsTYspectrum()" << result->tag;
    ready = true;
    result->vaChannelSpectrum.resize( scope->spectrum.size() );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelGraph &graphSpectrum = result->vaChannelSpectrum[ channel ];
        const SampleValues &sampleValues = useSpecSamplesOf( channel, result, scope );

        // Check if this channel is used and available at the data analyzer
        if ( sampleValues.samples.empty() ) {
            // Delete all vector arrays
            graphSpectrum.clear();
            continue;
        }
        // Check if the sample count has changed
        size_t sampleCount = sampleValues.samples.size();
        size_t neededSize = sampleCount * 2;

        // Set size directly to avoid reallocations
        graphSpectrum.reserve( neededSize );

        // What's the horizontal distance between sampling points?
        double horizontalFactor = sampleValues.interval / scope->horizontal.frequencybase;

        // Fill vector array
        std::vector< double >::const_iterator dataIterator = sampleValues.samples.begin();
        const double magnitude = scope->spectrum[ channel ].magnitude;
        const double offset = scope->spectrum[ channel ].offset;

        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            graphSpectrum.push_back( QVector3D( float( position * horizontalFactor - DIVS_TIME / 2 ),
                                                float( *dataIterator++ / magnitude + offset ), 0.0f ) );
        }
    }
}


void GraphGenerator::generateGraphsXY( PPresult *result ) {
    if ( scope->verboseLevel > 5 )
        qDebug() << "     GraphGenerator::generateGraphsXY()" << result->tag;
    result->vaChannelVoltage.resize( scope->voltage.size() );

    // Delete all spectrum graphs
    for ( ChannelGraph &data : result->vaChannelSpectrum )
        data.clear();

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
        if ( !xSamples.samples.size() || !ySamples.samples.size() ) {
            result->vaChannelVoltage[ xChannel ].clear();
            result->vaChannelVoltage[ yChannel ].clear();
            continue;
        }

        // Check if the sample count has changed
        const size_t sampleCount = std::min( xSamples.samples.size(), ySamples.samples.size() );
        ChannelGraph &graphXY = result->vaChannelVoltage[ yChannel ]; // color of y channel
        graphXY.reserve( sampleCount * 2 );

        // Fill vector array
        std::vector< double >::const_iterator xIterator = xSamples.samples.begin();
        std::vector< double >::const_iterator yIterator = ySamples.samples.begin();
        const double xGain = scope->gain( xChannel );
        const double yGain = scope->gain( yChannel );
        const double xOffset = ( scope->trigger.position - 0.5 ) * DIVS_TIME;
        const double yOffset = scope->voltage[ yChannel ].offset;

        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            graphXY.push_back( QVector3D( float( *xIterator++ / xGain + xOffset ), float( *yIterator++ / yGain + yOffset ), 0.0 ) );
        }
    }
}
