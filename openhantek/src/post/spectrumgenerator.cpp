// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include <QColor>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QToolTip>

#include "ppresult.h"
#include "spectrumgenerator.h"


#include "dsosettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"


/// \brief Analyzes the data from the dso.
SpectrumGenerator::SpectrumGenerator( const DsoSettingsScope *scope, const DsoSettingsAnalysis *analysis )
    : scope( scope ), analysis( analysis ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " SpectrumGenerator::SpectrumGenerator()";
}


SpectrumGenerator::~SpectrumGenerator() {
    if ( scope->verboseLevel > 1 )
        qDebug() << " SpectrumGenerator::~SpectrumGenerator()";
    if ( analysis->reuseFftPlan ) {
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


// besseli0() and Kaiser calculation from "SigPack - the C++ signal processing library"
// http://sigpack.sourceforge.net/window_8h_source.html
static double besseli0( double x ) {
    double y = 1.0, s = 1.0, x2 = x * x, n = 1.0;
    while ( s > y * 1.0e-9 ) {
        s *= x2 / 4.0 / ( n * n );
        y += s;
        n += 1;
    }
    return y;
}


void SpectrumGenerator::process( PPresult *result ) {
    // Calculate frequencies and spectrums

    if ( scope->verboseLevel > 4 )
        qDebug() << "    SpectrumGenerator::process()" << result->tag;

    // we use correctly aligned input and output data structures for fft
    // we use "fftw_alloc_real()" and "fftw_free()" to handle these arrays dynamically
    // these pointers are used during "process()"
    double *fftWindowedValues = nullptr;
    double *fftHcSpectrum = nullptr;
    double *fftPowerSpectrum = nullptr;
    double *fftAutoCorrelation = nullptr;

    for ( ChannelID channel = 0; channel < result->channelCount(); ++channel ) {
        DataChannel *const channelData = result->modifiableData( channel );

        if ( channelData->voltage.samples.empty() ) {
            // Clear unused channels
            channelData->spectrum.interval = 0;
            channelData->spectrum.samples.clear();
            continue;
        }
        int sampleCount = int( channelData->voltage.samples.size() );
        if ( scope->verboseLevel > 5 )
            qDebug() << "     SpectrumGenerator::process()" << channel << "sampleCount:" << sampleCount;

        // persistent window function, (re)build in case of changes only
        if ( previousWindowFunction != analysis->spectrumWindow || window.size() != size_t( sampleCount ) ) {
            // Calculate new window vector
            if ( scope->verboseLevel > 5 )
                qDebug() << "     SpectrumGenerator::process() calculate new window";
            previousWindowFunction = analysis->spectrumWindow;
            window.resize( size_t( sampleCount ) );

            // Theory:
            // Harris, Fredric J. (Jan 1978):
            // "On the use of Windows for Harmonic Analysis with the Discrete Fourier Transform".
            // Proceedings of the IEEE. 66 (1): 51–83. Bibcode:1978IEEEP..66...51H.
            // CiteSeerX 10.1.1.649.9880. doi:10.1109/PROC.1978.10837. S2CID 426548.
            // The fundamental 1978 paper on FFT windows by Harris, which specified many windows
            // and introduced key metrics used to compare them.
            // http://web.mit.edu/xiphmont/Public/windows.pdf

            double N = sampleCount - 1; // most window functions work for 0 <= n <= N
            // scale all windows to display 1 Veff as 0 dBu reference level.
            double area = 0.0; // calculate area under window fkt
            auto pW = window.begin();
            switch ( analysis->spectrumWindow ) {
            case Dso::WindowFunction::HANN:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.5 * ( 1.0 - cos( 2.0 * M_PI * n / N ) );
                break;
            case Dso::WindowFunction::HAMMING: {
                double a0 = 0.54; // approximation of a0 = 25.0 / 46.0
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = a0 - ( 1 - a0 ) * cos( 2.0 * M_PI * n / N );
                break;
            }
            case Dso::WindowFunction::COSINE:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = sin( M_PI * n / N );
                break;
            case Dso::WindowFunction::LANCZOS:
                for ( int n = 0; n < sampleCount; ++n ) {
                    double sincParameter = ( 2.0 * n / N - 1.0 ) * M_PI;
                    if ( bool( sincParameter ) )
                        area += *pW++ = sin( sincParameter ) / sincParameter;
                    else
                        area += *pW++ = 1;
                }
                break;
            case Dso::WindowFunction::TRIANGULAR: // same with N+1
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 2.0 / sampleCount * ( sampleCount / 2 - std::abs( n - N / 2.0 ) );
                break;
            case Dso::WindowFunction::BARTLETT: // the original triangle
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 2.0 / N * ( N / 2 - std::abs( n - N / 2.0 ) );
                break;
            case Dso::WindowFunction::BARTLETT_HANN:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.62 - 0.48 * std::abs( n / N - 0.5 ) - 0.38 * cos( 2.0 * M_PI * n / N );
                break;
            case Dso::WindowFunction::GAUSS: {
                const double sigma = 0.3;
                for ( int n = 0; n < sampleCount; ++n ) {
                    double w = ( n - N / 2.0 ) / ( sigma * N / 2.0 );
                    w *= w;
                    area += *pW++ = exp( -w / 2 );
                }
                break;
            }
            case Dso::WindowFunction::KAISER: {
                const double beta = M_PI * 2.75; // β = πα
                double bb = besseli0( beta );
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = besseli0( beta * sqrt( 4.0 * n * ( N - n ) ) / ( N ) ) / bb;
                break;
            }
            case Dso::WindowFunction::BLACKMAN: {
                const double alpha = 0.16;
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = ( 1 - alpha ) / 2 - 0.5 * cos( 2.0 * M_PI * n / N ) + alpha / 2 * cos( 4.0 * M_PI * n / N );
                break;
            }
            case Dso::WindowFunction::NUTTALL:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.355768 - 0.487396 * cos( 2 * M_PI * n / N ) + 0.144232 * cos( 4 * M_PI * n / N ) -
                                    0.012604 * cos( 6 * M_PI * n / N );
                break;
            case Dso::WindowFunction::BLACKMAN_HARRIS:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.35875 - 0.48829 * cos( 2 * M_PI * n / N ) + 0.14128 * cos( 4 * M_PI * n / N ) -
                                    0.01168 * cos( 6 * M_PI * n / N );
                break;
            case Dso::WindowFunction::BLACKMAN_NUTTALL:
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.3635819 - 0.4891775 * cos( 2 * M_PI * n / N ) + 0.1365995 * cos( 4 * M_PI * n / N ) -
                                    0.0106411 * cos( 6 * M_PI * n / N );
                break;
            case Dso::WindowFunction::FLATTOP: // wikipedia.de
                for ( int n = 0; n < sampleCount; ++n )
                    area += *pW++ = 0.216 - 0.417 * cos( 2 * M_PI * n / N ) + 0.277 * cos( 4 * M_PI * n / N ) -
                                    0.084 * cos( 6 * M_PI * n / N ) + 0.007 * cos( 8 * M_PI * n / N );
                break;
            default: // Dso::WINDOW_RECTANGULAR
                for ( auto &w : window )
                    area += w = 1.0;
            }
            // weight is the area below the window function
            double windowScale = sampleCount / area; // normalise all windows equal to the rectangular window

            // DFT transforms a 1V sin(ωt) signal to 1 = 0 dB, RMS = 0.707 V = sqrt(0.5) V (-3dBV)
            // If we want to scale to 0 dBu = 0 dBm @ 600 Ω, RMS = 0.775V = sqrt(1 mW * 600 Ω)
            // we must scale by sqrt(0.5/0.6) = -2.2 dB
            windowScale *= sqrt( 0.5 ); // scale display to 0 dBV -> 1V RMS = 0dB
            // printf( "window %u, weight %g\n", (unsigned)postprocessing->spectrumWindow, weight );
            // scale the windowed samples
            for ( auto &w : window )
                w *= windowScale;
        }

        // Allocate the sample buffer (16byte aligned)
        fftWindowedValues = fftw_alloc_real( size_t( qMax( SAMPLESIZE, sampleCount ) ) );
        if ( nullptr == fftWindowedValues )
            break;

        // Set sampling interval
        channelData->spectrum.interval = 1.0 / channelData->voltage.interval / double( sampleCount );

        // Number of real/complex samples
        int dftLength = sampleCount / 2;

        // Reallocate memory for samples if the sample count has changed
        channelData->spectrum.samples.resize( size_t( sampleCount ) );

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
        if ( right >= sampleCount )
            right = sampleCount - 1;
        for ( int position = left; // left side of trace
              position <= right;   // right side
              ++position ) {
            if ( channelData->voltage.samples[ unsigned( position ) ] < min )
                min = channelData->voltage.samples[ unsigned( position ) ];
            if ( channelData->voltage.samples[ unsigned( position ) ] > max )
                max = channelData->voltage.samples[ unsigned( position ) ];
        }
        channelData->vmin = min;
        channelData->vmax = max;
        // channelData->vpp = max - min;

        // calculate the average value
        double dc = 0.0;
        for ( auto &oneSample : channelData->voltage.samples )
            dc += oneSample;
        dc /= double( sampleCount );
        channelData->dc = dc;

        // now strip DC bias, calculate rms of AC component and apply window for fft to AC component
        double ac2 = 0.0;
        auto voltageIterator = channelData->voltage.samples.begin();
        auto windowIterator = window.begin();
        double *pfftW = fftWindowedValues;
        for ( int position = 0; position < sampleCount; ++position ) {
            double ac_sample = *voltageIterator++ - dc;
            ac2 += ac_sample * ac_sample;
            *pfftW++ = *windowIterator++ * ac_sample;
        }
        ac2 /= double( sampleCount );             // AC²
        channelData->ac = sqrt( ac2 );            // rms of AC component
        channelData->rms = sqrt( dc * dc + ac2 ); // total rms = U eff
        channelData->dB = 20.0 * log10( channelData->rms ) - scope->analysis.spectrumReference;
        channelData->pulseWidth1 = result->pulseWidth1;
        channelData->pulseWidth2 = result->pulseWidth2;

        // Do discrete real to half-complex transformation
        // Record length should be multiple of 2, 3, 5: done, is 10000 = 2^a * 5^b
        fftHcSpectrum = fftw_alloc_real( size_t( std::max( SAMPLESIZE, sampleCount ) ) );
        if ( nullptr == fftHcSpectrum ) // error
            break;
        if ( analysis->reuseFftPlan ) {    // build one optimized plan and reuse it for all transformations
            if ( nullptr == fftPlan_R2HC ) // not yet created, do it now (this takes some time)
                fftPlan_R2HC = fftw_plan_r2r_1d( sampleCount, fftWindowedValues, fftHcSpectrum, FFTW_R2HC, FFTW_MEASURE );
            fftw_execute_r2r( fftPlan_R2HC, fftWindowedValues, fftHcSpectrum ); // but it will run faster
        } else { // build a more generic plan, this takes much less time than the optimized plan
            fftPlan_R2HC = fftw_plan_r2r_1d( sampleCount, fftWindowedValues, fftHcSpectrum, FFTW_R2HC, FFTW_ESTIMATE );
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

        // create powerSpectrum in spectrum.samples (display) and a copy of it in powerSpectrum (for iDFT)
        // because hc2r iDFT destroys spectrum input
        const double norm = 1.0 / dftLength / dftLength;
        fftPowerSpectrum = fftWindowedValues; // "rename" the fftw array, will be reused as input for the iDFT
        fftWindowedValues = nullptr;          // invalidate the old pointer

        int position;
        // correct the (half-)complex values in hcSpectrum
        // (1st part real forward), (2nd part imag backwards) -> magnitude
        double const *fwd = fftHcSpectrum;                   // forward "iterator"
        double const *rev = fftHcSpectrum + sampleCount - 1; // reverse "iterator"
        double *powerIterator = fftPowerSpectrum;
        auto spectrumIterator = channelData->spectrum.samples.begin(); // this shall be displayed later
        // convert half-complex to magnitude square into spectrum.samples and into powerSpectrum
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
        channelData->spectrum.samples.resize( size_t( dftLength + 1 ) );

        // Complex values, all zero for autocorrelation
        for ( ++position; position < sampleCount; ++position ) {
            *powerIterator++ = 0;
        }

        // reuse the array, but "rename" it
        fftAutoCorrelation = fftHcSpectrum;
        fftHcSpectrum = nullptr;

        // Do half-complex to real inverse transformation -> autocorrelation
        if ( analysis->reuseFftPlan ) { // same as above for time -> spectrum
            if ( nullptr == fftPlan_HC2R )
                fftPlan_HC2R = fftw_plan_r2r_1d( sampleCount, fftPowerSpectrum, fftAutoCorrelation, FFTW_HC2R, FFTW_MEASURE );
            fftw_execute_r2r( fftPlan_HC2R, fftPowerSpectrum, fftAutoCorrelation );
        } else {
            fftw_plan fftPlan_HC2R =
                fftw_plan_r2r_1d( sampleCount, fftPowerSpectrum, fftAutoCorrelation, FFTW_HC2R, FFTW_ESTIMATE );
            fftw_execute( fftPlan_HC2R );
            fftw_destroy_plan( fftPlan_HC2R );
            fftPlan_HC2R = nullptr;
        }
        // content was destroyed during iFFT, free the memory
        fftw_free( fftPowerSpectrum );
        fftPowerSpectrum = nullptr;

        // Get the frequency from the correlation results
        int peakCorrPos = 0;
        double minCorr = 0;
        double maxCorr = 0;
        int maxCorrPos = 0;
        // search from right to left for a max and remember this if a following min corr (<0) is found
        for ( position = unsigned( sampleCount ) / 2; position > 1; --position ) { // go down to get leftmost peak (= max freq)
            if ( fftAutoCorrelation[ position ] > maxCorr ) {                      // find (local) max
                maxCorr = fftAutoCorrelation[ position ];
                maxCorrPos = position;
                minCorr = 0; // reset minimum to start new min search
                // printf( "max %d: %g\n", position, maxCorr );
            } else if ( fftAutoCorrelation[ position ] < minCorr ) { // search for local min
                minCorr = fftAutoCorrelation[ position ];
                maxCorr = 0; // reset max to start new max search
                peakCorrPos = maxCorrPos;
                // printf( "min %d: %g\n", position, minCorr );
            }
        }
        fftw_free( fftAutoCorrelation );
        fftAutoCorrelation = nullptr;

        // Finally calculate the real spectrum (it's also used for frequency calculation)
        // Convert values into dB (Relative to the reference level 0 dBV = 1V eff)
        double offset = -scope->analysis.spectrumReference - 20 * log10( dftLength );
        double offsetLimit = analysis->spectrumLimit; // - scope->analysis.spectrumReference;
        double peakSpectrum = offsetLimit;            // get a start value for peak search
        int peakFreqPos = 0;                          // initial position of max spectrum peak
        position = 0;
        min = INT_MAX;
        max = INT_MIN;
        for ( auto &oneSample : channelData->spectrum.samples ) {
            // spectrum is power spectrum, but show amplitude spectrum -> 10 * log...
            double value = 10 * log10( oneSample ) + offset;
            // Check if this value has to be limited
            if ( value < offsetLimit )
                value = offsetLimit;
            oneSample = value;
            // detect frequency peak
            if ( value > peakSpectrum ) {
                peakSpectrum = value;
                peakFreqPos = position;
            }
            if ( value < min )
                min = value;
            if ( value > max )
                max = value;
            ++position;
        }
        channelData->dBmin = min;
        channelData->dBmax = max;

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
        if ( scope->analysis.showNoteValue )
            channelData->note = calculateNote( channelData->frequency );
        else
            channelData->note = "";
        // calculate the total harmonic distortion of the signal (optional)
        // according IEEE method: THD = sqrt( power_of_harmonics / power_of_fundamental )
        if ( scope->analysis.calculateTHD ) { // set in menu Oscilloscope/Settings/Analysis
            channelData->thd = -1;            // invalid unless calculation is ok
            double f1 = channelData->frequency / channelData->spectrum.interval;
            if ( f1 >= 1 ) { // position of fundamental frequency is usable
                // get power of fundamental frequency
                double p1 = pow( 10, channelData->spectrum.samples[ unsigned( round( f1 ) ) ] / 10 );
                if ( p1 > 0 ) {
                    double pn = 0.0;                                     // sum of power of harmonics
                    for ( double fn = 2 * f1; fn < dftLength; fn += f1 ) // iterate over all harmonics
                        pn += pow( 10, channelData->spectrum.samples[ unsigned( round( fn ) ) ] / 10 );
                    channelData->thd = sqrt( pn / p1 );
                    if ( scope->verboseLevel > 5 )
                        qDebug() << "     SpectrumGenerator::process() THD" << channel << p1 << pn << channelData->thd;
                    // printf( "%g %g %g %% THD\n", p1, pn, channelData->thd );
                }
            }
        }
    }
    // free the memory used for fft unless already done ("fftw_free( nullptr )" is a no-op)
    fftw_free( fftWindowedValues );
    fftw_free( fftHcSpectrum );
    fftw_free( fftPowerSpectrum );
    fftw_free( fftAutoCorrelation );
}


const QString &SpectrumGenerator::calculateNote( double frequency ) {
    note = "";
    if ( frequency > 10 && frequency < 24000 ) { // audio frequencies
        const std::vector< QString > notes = { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };
        double f = fmod( 12 * log2( frequency / 440.0 ) + 120, 12.0 );
        int n = int( floor( f + 0.5 ) );
        f -= double( n );
        if ( n == 12 )
            n = 0;
        int ct = int( 100.0 * f ); // deviation from pure tone in cent
        if ( ct )
            note = QString( "♪ %1%2%3" ).arg( notes[ size_t( n ) ] ).arg( ct < 0 ? "" : "+" ).arg( ct );
        else
            note = QString( "♪ %1" ).arg( notes[ size_t( n ) ] ); // pure tone
    }
    return note;
}
