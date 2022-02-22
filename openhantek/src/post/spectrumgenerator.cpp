// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QColor>
#include <QDebug>
#include <QMutex>
#include <QTimer>

#include "ppresult.h"
#include "spectrumgenerator.h"


#include "dsosettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"


/// \brief Analyzes the data from the dso.
SpectrumGenerator::SpectrumGenerator( const DsoSettingsScope *scope, const DsoSettingsPostProcessing *post )
    : scope( scope ), post( post ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " SpectrumGenerator::SpectrumGenerator()";
}


SpectrumGenerator::~SpectrumGenerator() {
    if ( scope->verboseLevel > 1 )
        qDebug() << " SpectrumGenerator::~SpectrumGenerator()";
    if ( windowBuffer )
        fftw_free( windowBuffer );
    if ( post->reuseFftPlan ) {
        if ( fftPlan_R2HC ) {
            fftw_destroy_plan( fftPlan_R2HC );
            fftPlan_R2HC = nullptr;
        }
        if ( fftPlan_HC2R ) {
            fftw_destroy_plan( fftPlan_HC2R );
            fftPlan_HC2R = nullptr;
        }
    }
}


void SpectrumGenerator::process( PPresult *result ) {
    // Calculate frequencies and spectrums

    if ( scope->verboseLevel > 4 )
        qDebug() << "    SpectrumGenerator::process()" << result->tag;

    for ( ChannelID channel = 0; channel < result->channelCount(); ++channel ) {
        DataChannel *const channelData = result->modifiableData( channel );

        if ( channelData->voltage.sample.empty() ) {
            // Clear unused channels
            channelData->spectrum.interval = 0;
            channelData->spectrum.sample.clear();
            continue;
        }
        // Calculate new window
        // scale all windows to display 1 Veff as 0 dBu reference level.
        size_t sampleCount = channelData->voltage.sample.size();
        if ( !windowBuffer || lastWindow != post->spectrumWindow || lastRecordLength != sampleCount ) {
            if ( windowBuffer )
                fftw_free( windowBuffer );
            windowBuffer = fftw_alloc_real( sampleCount );
            lastRecordLength = unsigned( sampleCount );

            unsigned int windowEnd = lastRecordLength - 1;
            lastWindow = post->spectrumWindow;
            double weight = 0.0; // calculate area under window fkt
            switch ( post->spectrumWindow ) {
            case Dso::WindowFunction::HAMMING:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.54 - 0.46 * cos( 2.0 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::HANN:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.5 * ( 1.0 - cos( 2.0 * M_PI * windowPosition / windowEnd ) );
                break;
            case Dso::WindowFunction::COSINE:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = sin( M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::LANCZOS:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition ) {
                    double sincParameter = ( 2.0 * windowPosition / windowEnd - 1.0 ) * M_PI;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

                    if ( sincParameter == 0 )
                        weight += *( windowBuffer + windowPosition ) = 1;
                    else
                        weight += *( windowBuffer + windowPosition ) = sin( sincParameter ) / sincParameter;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
                }
                break;
            case Dso::WindowFunction::BARTLETT:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) =
                        2.0 / windowEnd * ( windowEnd / 2 - std::abs( double( windowPosition - windowEnd / 2.0 ) ) );
                break;
            case Dso::WindowFunction::TRIANGULAR:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) =
                        2.0 / lastRecordLength * ( lastRecordLength / 2 - std::abs( double( windowPosition - windowEnd / 2.0 ) ) );
                break;
            case Dso::WindowFunction::GAUSS: {
                const double sigma = 0.5;
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition ) {
                    double w = ( double( windowPosition ) - lastRecordLength / 2.0 ) / ( sigma * lastRecordLength / 2.0 );
                    w *= w;
                    weight += *( windowBuffer + windowPosition ) = exp( -w );
                }
            } break;
            case Dso::WindowFunction::BARTLETTHANN:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.62 -
                                                                   0.48 * std::abs( double( windowPosition / windowEnd - 0.5 ) ) -
                                                                   0.38 * cos( 2.0 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::BLACKMAN: {
                double alpha = 0.16;
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = ( 1 - alpha ) / 2 -
                                                                   0.5 * cos( 2.0 * M_PI * windowPosition / windowEnd ) +
                                                                   alpha / 2 * cos( 4.0 * M_PI * windowPosition / windowEnd );
            } break;
            // case Dso::WindowFunction::WINDOW_KAISER:
            //     TODO WINDOW_KAISER
            //     corr = dB( 0 );
            //     double alpha = 3.0;
            //     for(unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
            //         weight += *(window + windowPosition) = ...;
            //     break;
            case Dso::WindowFunction::NUTTALL:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.355768 -
                                                                   0.487396 * cos( 2 * M_PI * windowPosition / windowEnd ) +
                                                                   0.144232 * cos( 4 * M_PI * windowPosition / windowEnd ) -
                                                                   0.012604 * cos( 6 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::BLACKMANHARRIS:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.35875 -
                                                                   0.48829 * cos( 2 * M_PI * windowPosition / windowEnd ) +
                                                                   0.14128 * cos( 4 * M_PI * windowPosition / windowEnd ) -
                                                                   0.01168 * cos( 6 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::BLACKMANNUTTALL:
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 0.3635819 -
                                                                   0.4891775 * cos( 2 * M_PI * windowPosition / windowEnd ) +
                                                                   0.1365995 * cos( 4 * M_PI * windowPosition / windowEnd ) -
                                                                   0.0106411 * cos( 6 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::FLATTOP: // wikipedia.de
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 1.0 - 1.93 * cos( 2 * M_PI * windowPosition / windowEnd ) +
                                                                   1.29 * cos( 4 * M_PI * windowPosition / windowEnd ) -
                                                                   0.388 * cos( 6 * M_PI * windowPosition / windowEnd ) +
                                                                   0.028 * cos( 8 * M_PI * windowPosition / windowEnd );
                break;
            default: // Dso::WINDOW_RECTANGULAR
                for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                    weight += *( windowBuffer + windowPosition ) = 1.0;
            }
            // weight is the area below the window function
            weight = lastRecordLength / weight; // normalise all windows equal to the rectangular window

            // DFT transforms a 1V sin(ωt) signal to 1 = 0 dB, RMS = 0.707 V = sqrt(0.5) V (-3dBV)
            // If we want to scale to 0 dBu = 0 dBm @ 600 Ω, RMS = 0.775V = sqrt(1 mW * 600 Ω)
            // we must scale by sqrt(0.5/0.6) = -2.2 dB
            weight *= sqrt( 0.5 ); // scale display to 0 dBV -> 1V RMS = 0dB
            // printf( "window %u, weight %g\n", (unsigned)postprocessing->spectrumWindow, weight );
            // scale the windowed samples
            for ( unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition )
                *( windowBuffer + windowPosition ) *= weight;
        }

        // Set sampling interval
        channelData->spectrum.interval = 1.0 / channelData->voltage.interval / double( sampleCount );

        // Number of real/complex samples
        unsigned int dftLength = unsigned( sampleCount ) / 2;

        // Reallocate memory for samples if the sample count has changed
        channelData->spectrum.sample.resize( sampleCount );

        // Create sample buffer
        windowedValues = fftw_alloc_real( sampleCount );
        // calculate the peak-to-peak value of the displayed part of trace
        double min = INT_MAX;
        double max = INT_MIN;
        double horizontalFactor = result->data( channel )->voltage.interval / scope->horizontal.timebase;
        unsigned dotsOnScreen = unsigned( DIVS_TIME / horizontalFactor + 0.99 ); // round up
        unsigned preTrigSamples = unsigned( scope->trigger.position * dotsOnScreen );
        int left = int( result->triggeredPosition ) - int( preTrigSamples ); // 1st sample to show
        int right = left + int( dotsOnScreen );                              // last sample to show
        if ( left < 0 )                                                      // trig pos or time/div was increased
            left = 0;                                                        // show as much as we have on left side
        // unsigned right = result->triggerPosition + DIVS_TIME * scope->horizontal.timebase / channelData->voltage.interval;
        if ( right >= int( sampleCount ) )
            right = int( sampleCount ) - 1;
        for ( int position = left; // left side of trace
              position <= right;   // right side
              ++position ) {
            if ( channelData->voltage.sample[ unsigned( position ) ] < min )
                min = channelData->voltage.sample[ unsigned( position ) ];
            if ( channelData->voltage.sample[ unsigned( position ) ] > max )
                max = channelData->voltage.sample[ unsigned( position ) ];
        }
        channelData->vpp = max - min;
        // printf( "dots = %d, Vpp = %g\n", dots, channelData->vpp );

        // calculate the average value
        double dc = 0.0;
        auto voltageIterator = channelData->voltage.sample.begin();
        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            dc += *voltageIterator++;
        }
        dc /= double( sampleCount );
        channelData->dc = dc;

        // now strip DC bias, calculate rms of AC component and apply window for fft to AC component
        double ac2 = 0.0;
        voltageIterator = channelData->voltage.sample.begin();
        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            double ac_sample = *voltageIterator++ - dc;
            ac2 += ac_sample * ac_sample;
            windowedValues[ position ] = windowBuffer[ position ] * ac_sample;
        }
        ac2 /= double( sampleCount );             // AC²
        channelData->ac = sqrt( ac2 );            // rms of AC component
        channelData->rms = sqrt( dc * dc + ac2 ); // total rms = U eff
        channelData->dB = 20.0 * log10( channelData->rms ) - post->spectrumReference;
        channelData->pulseWidth1 = result->pulseWidth1;
        channelData->pulseWidth2 = result->pulseWidth2;

        // Do discrete real to half-complex transformation
        // Record length should be multiple of 2, 3, 5: done, is 10000 = 2^a * 5^b
        hcSpectrum = fftw_alloc_real( sampleCount );
        if ( post->reuseFftPlan ) {        // build one optimized plan and reuse it for all transformations
            if ( nullptr == fftPlan_R2HC ) // not yet created, do it now (this takes some time)
                fftPlan_R2HC = fftw_plan_r2r_1d( int( sampleCount ), windowedValues, hcSpectrum, FFTW_R2HC, FFTW_MEASURE );
            fftw_execute_r2r( fftPlan_R2HC, windowedValues, hcSpectrum ); // but it will run faster
        } else { // build a more generic plan, this takes much less time than the optimized plan
            fftPlan_R2HC = fftw_plan_r2r_1d( int( sampleCount ), windowedValues, hcSpectrum, FFTW_R2HC, FFTW_ESTIMATE );
            fftw_execute( fftPlan_R2HC );      // use it once
            fftw_destroy_plan( fftPlan_R2HC ); // and destroy it
            fftPlan_R2HC = nullptr;            // no plan available;
        }
        // Do an autocorrelation to get the frequency of the signal
        // fft: f(t) o-- F(ω); calculate power spectrum |F(ω)|²
        // ifft: F(ω) ∙ F(ω) --o f(t) ⊗ f(t) (convolution of f(t) with f(t), i.e. autocorrelation)
        // HORO:
        // This is quite inaccurate at high frequencies due to the used algorithm:
        // as we do a autocorrelation the resolution at high frequencies is limited by voltagestep interval
        // e.g. at 6 MHz sampled with 30 MS/s we get correlation at time shift
        // of either 6 or 5 or 4 samples -> 30 MHz / 6 = 5.0 MHz ; 30 / 5 = 6.0 ; 30 / 4 = 7.5
        // in these cases use spectrum instead if peak position is too small.

        // create powerSpectrum in spectrum.sample (display) and a copy of it in powerSpectrum (for iDFT)
        // because hc2r iDFT destroys spectrum input
        const double norm = 1.0 / dftLength / dftLength;
        powerSpectrum = windowedValues; // rename the fftw array, will be the input for the iDFT
        windowedValues = nullptr;       // invalidate the old pointer

        unsigned int position;
        // correct the (half-)complex values in hcSpectrum
        // (1st part real forward), (2nd part imag backwards) -> magnitude
        double const *fwd = hcSpectrum;                   // forward "iterator"
        double const *rev = hcSpectrum + sampleCount - 1; // reverse "iterator"
        double *powerIterator = powerSpectrum;
        auto spectrumIterator = channelData->spectrum.sample.begin(); // this shall be displayed later
        // convert half-complex to magnitude square into spectrum.sample and into powerSpectrum
        *spectrumIterator = *fwd * *fwd;
        *powerIterator++ = *spectrumIterator++ * norm;
        ++fwd; // spectrum[0] is only real
        for ( position = 1; position < dftLength; ++position ) {
            *spectrumIterator = ( *fwd * *fwd + *rev * *rev );
            *powerIterator++ = *spectrumIterator++ * norm;
            ++fwd;
            --rev;
        }
        *spectrumIterator = *fwd * *fwd;
        *powerIterator++ = *spectrumIterator++ * norm;

        // skip mirrored 2nd half (-1) of result spectrum
        channelData->spectrum.sample.resize( dftLength + 1 );

        // Complex values, all zero for autocorrelation
        for ( ++position; position < sampleCount; ++position ) {
            *powerIterator++ = 0;
        }

        // free the memory of hcSpectrum
        if ( hcSpectrum )
            fftw_free( hcSpectrum );
        hcSpectrum = nullptr;

        // Do half-complex to real inverse transformation -> autocorrelation
        // std::unique_ptr< double[] > correlation = std::unique_ptr< double[] >( new double[ sampleCount ] );
        autoCorrelation = fftw_alloc_real( sampleCount );

        if ( post->reuseFftPlan ) { // same as above for time -> spectrum
            if ( nullptr == fftPlan_HC2R )
                fftPlan_HC2R = fftw_plan_r2r_1d( int( sampleCount ), powerSpectrum, autoCorrelation, FFTW_HC2R, FFTW_MEASURE );
            fftw_execute_r2r( fftPlan_HC2R, powerSpectrum, autoCorrelation );
        } else {
            fftw_plan fftPlan_HC2R =
                fftw_plan_r2r_1d( int( sampleCount ), powerSpectrum, autoCorrelation, FFTW_HC2R, FFTW_ESTIMATE );
            fftw_execute( fftPlan_HC2R );
            fftw_destroy_plan( fftPlan_HC2R );
            fftPlan_HC2R = nullptr;
        }
        // content was destroyed during iFFT, free the memory
        if ( powerSpectrum )
            fftw_free( powerSpectrum );
        powerSpectrum = nullptr;

        // Get the frequency from the correlation results
        unsigned int peakCorrPos = 0;
        double minCorr = 0;
        double maxCorr = 0;
        unsigned maxCorrPos = 0;
        // search from right to left for a max and remember this if a following min corr (<0) is found
        for ( position = unsigned( sampleCount ) / 2; position > 1; --position ) { // go down to get leftmost peak (= max freq)
            if ( autoCorrelation[ position ] > maxCorr ) {                         // find (local) max
                maxCorr = autoCorrelation[ position ];
                maxCorrPos = position;
                minCorr = 0; // reset minimum to start new min search
                // printf( "max %d: %g\n", position, maxCorr );
            } else if ( autoCorrelation[ position ] < minCorr ) { // search for local min
                minCorr = autoCorrelation[ position ];
                maxCorr = 0; // reset max to start new max seach
                peakCorrPos = maxCorrPos;
                // printf( "min %d: %g\n", position, minCorr );
            }
        }
        if ( autoCorrelation )
            fftw_free( autoCorrelation );
        autoCorrelation = nullptr;

        // Finally calculate the real spectrum (it's also used for frequency calculation)
        // Convert values into dB (Relative to the reference level 0 dBV = 1V eff)
        double offset = -post->spectrumReference - 20 * log10( dftLength );
        double offsetLimit = post->spectrumLimit - post->spectrumReference;
        double peakSpectrum = offsetLimit; // get a start value for peak search
        unsigned int peakFreqPos = 0;      // initial position of max spectrum peak
        position = 0;
        for ( auto spectrumIterator = channelData->spectrum.sample.begin(), spectrumEnd = channelData->spectrum.sample.end();
              spectrumIterator != spectrumEnd; ++spectrumIterator, ++position ) {
            // spectrum is power spectrum, but show amplitude spectrum -> 10 * log...
            double value = 10 * log10( *spectrumIterator ) + offset;
            // Check if this value has to be limited
            if ( value < offsetLimit )
                value = offsetLimit;
            *spectrumIterator = value;
            // detect frequency peak
            if ( value > peakSpectrum ) {
                peakSpectrum = value;
                peakFreqPos = position;
            }
        }

        // Calculate both peak frequencies (correlation and spectrum) in Hz
        double pF = channelData->spectrum.interval * peakFreqPos;
        double pC = 1.0 / ( channelData->voltage.interval * peakCorrPos );
        if ( scope->verboseLevel > 5 )
            qDebug() << "     SpectrumGenerator::process()" << channel << "freq:" << peakFreqPos << pF << "corr:" << peakCorrPos
                     << pC;
        if ( peakFreqPos > peakCorrPos // use frequency result if it is more granular than correlation
             || peakFreqPos > 100      // or at least if it is granular enough (+- 1% resolution)
             || peakCorrPos < 100 || peakCorrPos > sampleCount / 4 ) { // or if correlation is out of safe range
            channelData->frequency = pF;
        } else { // otherwise fall back to correlation
            channelData->frequency = pC;
        }

        // calculate the total harmonic distortion of the signal (optional)
        // THD = sqrt( power_of_harmonics / power_of_fundamental )
        if ( scope->analysis.calculateTHD ) { // set in menu Oscilloscope/Settings/Analysis
            channelData->thd = -1;            // invalid unless calculation is ok
            double f1 = channelData->frequency / channelData->spectrum.interval;
            if ( f1 >= 1 ) { // position of fundamental frequency is usable
                // get power of fundamental frequency
                double p1 = pow( 10, channelData->spectrum.sample[ unsigned( round( f1 ) ) ] / 10 );
                if ( p1 > 0 ) {
                    double pn = 0.0;                                     // sum of power of harmonics
                    for ( double fn = 2 * f1; fn < dftLength; fn += f1 ) // iterate over all harmonics
                        pn += pow( 10, channelData->spectrum.sample[ unsigned( round( fn ) ) ] / 10 );
                    channelData->thd = sqrt( pn / p1 );
                    if ( scope->verboseLevel > 5 )
                        qDebug() << "     SpectrumGenerator::process() THD" << channel << p1 << pn << channelData->thd;
                    // printf( "%g %g %g %% THD\n", p1, pn, channelData->thd );
                }
            }
        }
    }
}
