// SPDX-License-Identifier: GPL-2.0+

#define _USE_MATH_DEFINES
#include <cmath>

#include <QColor>
#include <QMutex>
#include <QTimer>

#include <fftw3.h>

#include "spectrumgenerator.h"

#include "glscope.h"
#include "settings.h"
#include "utils/printutils.h"

/// \brief Analyzes the data from the dso.
SpectrumGenerator::SpectrumGenerator(const DsoSettingsScope *scope, const DsoSettingsPostProcessing *postprocessing)
    : scope(scope), postprocessing(postprocessing) {}

SpectrumGenerator::~SpectrumGenerator() {
    if (lastWindowBuffer) fftw_free(lastWindowBuffer);
}

//static double dB( double dB ) { return pow( 10.0, dB/20.0 ); }

void SpectrumGenerator::process(PPresult *result) {
    // Calculate frequencies and spectrums
    for (ChannelID channel = 0; channel < result->channelCount(); ++channel) {
        DataChannel *const channelData = result->modifyData(channel);

        if (channelData->voltage.sample.empty()) {
            // Clear unused channels
            channelData->spectrum.interval = 0;
            channelData->spectrum.sample.clear();
            continue;
        }
        // Calculate new window
        // scale all windows to display 1 Veff as 0 dBu reference level.
        size_t sampleCount = channelData->voltage.sample.size();
        if (!lastWindowBuffer || lastWindow != postprocessing->spectrumWindow || lastRecordLength != sampleCount) {
            if (lastWindowBuffer) fftw_free(lastWindowBuffer);
            lastWindowBuffer = fftw_alloc_real(sampleCount);
            lastRecordLength = (unsigned)sampleCount;

            unsigned int windowEnd = lastRecordLength - 1;
            lastWindow = postprocessing->spectrumWindow;
            double weight = 0.0; //calculate area under window fkt
            switch (postprocessing->spectrumWindow) {
            case Dso::WindowFunction::HAMMING:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.54 - 0.46 * cos( 2.0 * M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::HANN:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.5 * ( 1.0 - cos( 2.0 * M_PI * windowPosition / windowEnd ) );
                break;
            case Dso::WindowFunction::COSINE:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = sin( M_PI * windowPosition / windowEnd );
                break;
            case Dso::WindowFunction::LANCZOS:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition) {
                    double sincParameter = (2.0 * windowPosition / windowEnd - 1.0) * M_PI;
                    if (sincParameter == 0)
                        weight += *(lastWindowBuffer + windowPosition) = 1;
                    else
                        weight += *(lastWindowBuffer + windowPosition) = sin( sincParameter ) / sincParameter;
                }
                break;
            case Dso::WindowFunction::BARTLETT:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) =
                        2.0 / windowEnd * (windowEnd / 2 - std::abs((double)(windowPosition - windowEnd / 2.0)));
                break;
            case Dso::WindowFunction::TRIANGULAR:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) =
                        2.0 / lastRecordLength *
                        (lastRecordLength / 2 - std::abs((double)(windowPosition - windowEnd / 2.0)));
                break;
            case Dso::WindowFunction::GAUSS: {
                const double sigma = 0.5;
                double w;
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition) {
                    w = ( (double)windowPosition - lastRecordLength / 2.0 ) / ( sigma * lastRecordLength / 2.0 );
                    w *= w;
                    weight += *(lastWindowBuffer + windowPosition) = exp( -w );
                }
            } break;
            case Dso::WindowFunction::BARTLETTHANN:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.62 -
                                                             0.48 * std::abs((double)(windowPosition / windowEnd - 0.5)) -
                                                             0.38 * cos(2.0 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WindowFunction::BLACKMAN: {
                double alpha = 0.16;
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = (1 - alpha) / 2 -
                                                           0.5 * cos(2.0 * M_PI * windowPosition / windowEnd) +
                                                           alpha / 2 * cos(4.0 * M_PI * windowPosition / windowEnd);
            } break;
            // case Dso::WindowFunction::WINDOW_KAISER:
            //     TODO WINDOW_KAISER
            //     corr = dB( 0 );
            //     double alpha = 3.0;
            //     for(unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
            //         weight += *(window + windowPosition) = ...;
            //     break;
            case Dso::WindowFunction::NUTTALL:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.355768 -
                                                             0.487396 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                             0.144232 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                             0.012604 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WindowFunction::BLACKMANHARRIS:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.35875 -
                                                             0.48829 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                             0.14128 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                             0.01168 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WindowFunction::BLACKMANNUTTALL:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 0.3635819 -
                                                             0.4891775 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                             0.1365995 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                             0.0106411 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WindowFunction::FLATTOP: // wikipedia.de
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 1.0 - 1.93 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                           1.29 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                           0.388 * cos(6 * M_PI * windowPosition / windowEnd) +
                                                           0.028 * cos(8 * M_PI * windowPosition / windowEnd);
                break;
            default: // Dso::WINDOW_RECTANGULAR
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    weight += *(lastWindowBuffer + windowPosition) = 1.0;
            }
            // weight is the area below the window function
            weight = lastRecordLength / weight; //normalise all windows equal to the rectangular window

            // DFT transforms a 1V sin(ωt) signal to 1 = 0 dB, RMS = 0.707 V = sqrt(0.5) V (-3dBV)
            // If we want to scale to 0 dBu = 0 dBm @ 600 Ω, RMS = 0.775V = sqrt(1 mW * 600 Ω)
            // we must scale by sqrt(0.5/0.6) = -2.2 dB
            weight *= sqrt(0.5); // scale display to 0 dBV -> 1V RMS = 0dB
            // printf( "window %u, weight %g\n", (unsigned)postprocessing->spectrumWindow, weight );
            // scale the windowed samples
            for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                *(lastWindowBuffer + windowPosition) *= weight;
        }

        // Set sampling interval
        channelData->spectrum.interval = 1.0 / channelData->voltage.interval / sampleCount;

        // Number of real/complex samples
        unsigned int dftLength = sampleCount / 2;

        // Reallocate memory for samples if the sample count has changed
        channelData->spectrum.sample.resize(sampleCount);

        // Create sample buffer and apply window
        std::unique_ptr<double[]> windowedValues = std::unique_ptr<double[]>(new double[sampleCount]);
        // calculate the average value
        double dc = 0.0;

        for (unsigned int position = 0; position < sampleCount; ++position) {
            dc += channelData->voltage.sample[position];
        }

        dc /= sampleCount;
        channelData->dc = dc;
        // now strip DC bias, calculate rms of AC component and apply window for fft to AC component
        double ac2 = 0.0;
        for (unsigned int position = 0; position < sampleCount; ++position) {
            double ac_sample = channelData->voltage.sample[position] - dc;
            ac2 += ac_sample * ac_sample;
            windowedValues[position] = lastWindowBuffer[position] * ac_sample;
        }
        ac2 /= sampleCount;
        channelData->ac = sqrt( ac2 ); // rms of AC component
        channelData->rms = sqrt( dc * dc + ac2 ); // total rms = U eff

        // Do discrete real to half-complex transformation
        /// \todo Check if record length is multiple of 2
        /// \todo Reuse plan and use FFTW_MEASURE to get fastest algorithm

        fftw_plan fftPlan;
        fftPlan = fftw_plan_r2r_1d(sampleCount, windowedValues.get(),
                                    &channelData->spectrum.sample.front(), FFTW_R2HC, FFTW_ESTIMATE);
        fftw_execute(fftPlan);
        fftw_destroy_plan(fftPlan);

        // Do an autocorrelation to get the frequency of the signal
        // HORO:
        // This is quite inaccurate at high frequencies due to the used algorithm:
        // as we do a autocorrelation the resolution at high frequencies is limited by voltagestep interval
        // e.g. at 6 MHz we get correlation at time shift of either 4 or 5 or 6 -> 5.0 / 6.0 / 7.5 MHz
        // use spectrum instead if peak position is too small.

        // create a copy of powerSpectrum because hc2r iDFT destroys spectrum input
        const double norm = 1.0 / dftLength / dftLength;
        std::unique_ptr<double[]> powerSpectrum = std::move(windowedValues);

        unsigned int position;
        // correct the (half-)complex values in spectrum (1st part real forward), (2nd part imag backwards) -> magnitude
        std::vector<double>::iterator fwd = channelData->spectrum.sample.begin();
        std::vector<double>::reverse_iterator rev = channelData->spectrum.sample.rbegin();
        // convert complex to magnitude square in place (*fwd) and into copy (powerSpectrum[])
        *fwd = *fwd * *fwd;
        powerSpectrum[ 0 ] = *fwd * norm;
        fwd++; // spectrum[0] is only real
        for ( position = 1; position < dftLength; ++position ) {
            *fwd = ( *fwd * *fwd + *rev * *rev );
            powerSpectrum[ position ] = *fwd * norm;
            fwd++;
            rev++;
        }
        *fwd = *fwd * *fwd;
        powerSpectrum[ position ] = *fwd * norm;
        fwd++;
        // Complex values, all zero for autocorrelation
        for (++position; position < sampleCount; ++position) {
            powerSpectrum[ position ] = 0;
        }
        // skip mirrored 2nd half of result spectrum
        channelData->spectrum.sample.resize( dftLength + 1 );

        // Do half-complex to real inverse transformation -> autocorrelation
        std::unique_ptr<double[]> correlation = std::unique_ptr<double[]>(new double[sampleCount]);
        fftPlan = fftw_plan_r2r_1d(sampleCount, powerSpectrum.get(), correlation.get(), FFTW_HC2R, FFTW_ESTIMATE);
        fftw_execute(fftPlan);
        fftw_destroy_plan(fftPlan);

        // Get the frequency from the correlation results
        double minimumCorrelation = correlation[0];
        double peakCorrelation = 0;
        unsigned int peakCorrPos = 0;

        for ( position = 1; position < sampleCount / 2; ++position ) {
            if ( correlation[position] > peakCorrelation && correlation[position] > minimumCorrelation ) {
                peakCorrelation = correlation[position];
                peakCorrPos = position;
            } else if (correlation[position] < minimumCorrelation)
                minimumCorrelation = correlation[position];
        }
        correlation.reset(nullptr);

        // Finally calculate the real spectrum (it's also used for frequency display)
        // Convert values into dB (Relative to the reference level 0 dBu = 1V eff)
        double offset = - postprocessing->spectrumReference - 20 * log10(dftLength);
        double offsetLimit = postprocessing->spectrumLimit - postprocessing->spectrumReference;
        double peakSpectrum = offsetLimit; // get a start value for peak search
        unsigned int peakFreqPos = 0; // initial position of max spectrum peak
        position = 0;
        for (std::vector<double>::iterator spectrumIterator = channelData->spectrum.sample.begin();
             spectrumIterator != channelData->spectrum.sample.end(); ++spectrumIterator) {
            // spectrum is power spectrum, but show amplitude spectrum -> 10 * log...
            double value = 10 * log10( *spectrumIterator ) + offset;
            // Check if this value has to be limited
            if (value < offsetLimit)
                value = offsetLimit;
            *spectrumIterator = value;
            // detect frequency peak
            if ( value > peakSpectrum  ) {
                peakSpectrum = value;
                peakFreqPos = position;
            }
            position++;
        }
        // Calculate both peak frequencies (correlation and spectrum) in Hz
        double pF = channelData->spectrum.interval * peakFreqPos;
        double pC = 1.0 / ( channelData->voltage.interval * peakCorrPos );
        //printf( "pF %u: %d  %g\n", channel, peakFreqPos, pF );
        //printf( "pC %u: %d  %g\n", channel, peakCorrPos, pC );
        if ( peakFreqPos > peakCorrPos || peakFreqPos > 100  // use frequency result if it is granular enough (+- 1%)
           || peakCorrPos < 100 || peakCorrPos > sampleCount / 2 // or correlation is out of safe range
           || pF > pC ) { // or frequency result is higher as correlation result (due to subharmonics)
            channelData->frequency = pF;
        } else { // fall back to correlation
            channelData->frequency = pC;
        }
    }
}
