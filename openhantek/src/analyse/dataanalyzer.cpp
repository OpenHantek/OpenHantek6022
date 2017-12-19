// SPDX-License-Identifier: GPL-2.0+

#define _USE_MATH_DEFINES
#include <cmath>

#include <QColor>
#include <QMutex>
#include <QTimer>

#include <fftw3.h>

#include "dataanalyzer.h"

#include "glscope.h"
#include "settings.h"
#include "utils/printutils.h"

std::unique_ptr<DataAnalyzerResult> DataAnalyzer::convertData(const DSOsamples *data, const DsoSettingsScope *scope) {
    QReadLocker locker(&data->lock);

    unsigned int channelCount = (unsigned int)scope->voltage.size();

    std::unique_ptr<DataAnalyzerResult> result =
        std::unique_ptr<DataAnalyzerResult>(new DataAnalyzerResult(channelCount));

    for (unsigned int channel = 0; channel < channelCount; ++channel) {
        DataChannel *const channelData = result->modifyData(channel);

        bool gotDataForChannel = channel < scope->physicalChannels && channel < (unsigned int)data->data.size() &&
                                 !data->data.at(channel).empty();
        bool isMathChannel = channel >= scope->physicalChannels &&
                             (scope->voltage[channel].used || scope->spectrum[channel].used) &&
                             result->channelCount() >= 2 && !result->data(0)->voltage.sample.empty() &&
                             !result->data(1)->voltage.sample.empty();

        if (!gotDataForChannel && !isMathChannel) {
            // Clear unused channels
            channelData->voltage.sample.clear();
            result->modifyData(scope->physicalChannels)->voltage.interval = 0;
            continue;
        }

        // Set sampling interval
        const double interval = 1.0 / data->samplerate;
        if (interval != channelData->voltage.interval) {
            channelData->voltage.interval = interval;
            if (data->append) // Clear roll buffer if the samplerate changed
                channelData->voltage.sample.clear();
        }

        unsigned int size;
        if (channel < scope->physicalChannels) {
            size = data->data.at(channel).size();
            if (data->append) size += channelData->voltage.sample.size();
            result->challengeMaxSamples(size);
        } else
            size = result->getMaxSamples();

        // Physical channels
        if (channel < scope->physicalChannels) {
            // Copy the buffer of the oscilloscope into the sample buffer
            if (data->append)
                channelData->voltage.sample.insert(channelData->voltage.sample.end(), data->data.at(channel).begin(),
                                                   data->data.at(channel).end());
            else
                channelData->voltage.sample = data->data.at(channel);
        } else { // Math channel
            // Resize the sample vector
            channelData->voltage.sample.resize(size);
            // Set sampling interval
            result->modifyData(scope->physicalChannels)->voltage.interval = result->data(0)->voltage.interval;

            // Resize the sample vector
            result->modifyData(scope->physicalChannels)
                ->voltage.sample.resize(
                    qMin(result->data(0)->voltage.sample.size(), result->data(1)->voltage.sample.size()));

            // Calculate values and write them into the sample buffer
            std::vector<double>::const_iterator ch1Iterator = result->data(0)->voltage.sample.begin();
            std::vector<double>::const_iterator ch2Iterator = result->data(1)->voltage.sample.begin();
            std::vector<double> &resultData = result->modifyData(scope->physicalChannels)->voltage.sample;
            for (std::vector<double>::iterator resultIterator = resultData.begin(); resultIterator != resultData.end();
                 ++resultIterator) {
                switch (scope->voltage[scope->physicalChannels].misc) {
                case Dso::MATHMODE_1ADD2:
                    *resultIterator = *ch1Iterator + *ch2Iterator;
                    break;
                case Dso::MATHMODE_1SUB2:
                    *resultIterator = *ch1Iterator - *ch2Iterator;
                    break;
                case Dso::MATHMODE_2SUB1:
                    *resultIterator = *ch2Iterator - *ch1Iterator;
                    break;
                }
                ++ch1Iterator;
                ++ch2Iterator;
            }
        }
    }
    return result;
}

/// \brief Analyzes the data from the dso.
DataAnalyzer::~DataAnalyzer() {
    if (window) fftw_free(window);
}

void DataAnalyzer::applySettings(DsoSettingsScope *scope) { this->scope = scope; }

void DataAnalyzer::setSourceData(const DSOsamples *data) { sourceData = data; }

std::unique_ptr<DataAnalyzerResult> DataAnalyzer::getNextResult() { return std::move(lastResult); }

void DataAnalyzer::samplesAvailable() {
    if (sourceData == nullptr) return;
    std::unique_ptr<DataAnalyzerResult> result = convertData(sourceData, scope);
    spectrumAnalysis(result.get(), lastWindow, lastRecordLength, window, scope);
    lastResult.swap(result);
    emit analyzed();
}

void DataAnalyzer::spectrumAnalysis(DataAnalyzerResult *result, Dso::WindowFunction &lastWindow,
                                    unsigned int lastRecordLength, double *&lastWindowBuffer,
                                    const DsoSettingsScope *scope) {
    // Calculate frequencies, peak-to-peak voltages and spectrums
    for (unsigned int channel = 0; channel < result->channelCount(); ++channel) {
        DataChannel *const channelData = result->modifyData(channel);

        if (channelData->voltage.sample.empty()) {
            // Clear unused channels
            channelData->spectrum.interval = 0;
            channelData->spectrum.sample.clear();
            continue;
        }

        // Calculate new window
        unsigned int sampleCount = channelData->voltage.sample.size();
        if (!lastWindowBuffer || lastWindow != scope->spectrumWindow || lastRecordLength != sampleCount) {
            if (lastWindowBuffer) fftw_free(lastWindowBuffer);
            lastWindowBuffer = fftw_alloc_real(sampleCount);
            lastRecordLength = sampleCount;

            unsigned int windowEnd = lastRecordLength - 1;
            lastWindow = scope->spectrumWindow;

            switch (scope->spectrumWindow) {
            case Dso::WINDOW_HAMMING:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.54 - 0.46 * cos(2.0 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_HANN:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.5 * (1.0 - cos(2.0 * M_PI * windowPosition / windowEnd));
                break;
            case Dso::WINDOW_COSINE:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = sin(M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_LANCZOS:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition) {
                    double sincParameter = (2.0 * windowPosition / windowEnd - 1.0) * M_PI;
                    if (sincParameter == 0)
                        *(lastWindowBuffer + windowPosition) = 1;
                    else
                        *(lastWindowBuffer + windowPosition) = sin(sincParameter) / sincParameter;
                }
                break;
            case Dso::WINDOW_BARTLETT:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) =
                        2.0 / windowEnd * (windowEnd / 2 - std::abs((double)(windowPosition - windowEnd / 2.0)));
                break;
            case Dso::WINDOW_TRIANGULAR:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) =
                        2.0 / lastRecordLength *
                        (lastRecordLength / 2 - std::abs((double)(windowPosition - windowEnd / 2.0)));
                break;
            case Dso::WINDOW_GAUSS: {
                double sigma = 0.4;
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) =
                        exp(-0.5 * pow(((windowPosition - windowEnd / 2) / (sigma * windowEnd / 2)), 2));
            } break;
            case Dso::WINDOW_BARTLETTHANN:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.62 -
                                                           0.48 * std::abs((double)(windowPosition / windowEnd - 0.5)) -
                                                           0.38 * cos(2.0 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_BLACKMAN: {
                double alpha = 0.16;
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = (1 - alpha) / 2 -
                                                           0.5 * cos(2.0 * M_PI * windowPosition / windowEnd) +
                                                           alpha / 2 * cos(4.0 * M_PI * windowPosition / windowEnd);
            } break;
            // case WINDOW_KAISER:
            // TODO WINDOW_KAISER
            // double alpha = 3.0;
            // for(unsigned int windowPosition = 0; windowPosition <
            // lastRecordLength; ++windowPosition)
            //*(window + windowPosition) = ;
            // break;
            case Dso::WINDOW_NUTTALL:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.355768 -
                                                           0.487396 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                           0.144232 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                           0.012604 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_BLACKMANHARRIS:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.35875 -
                                                           0.48829 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                           0.14128 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                           0.01168 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_BLACKMANNUTTALL:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 0.3635819 -
                                                           0.4891775 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                           0.1365995 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                           0.0106411 * cos(6 * M_PI * windowPosition / windowEnd);
                break;
            case Dso::WINDOW_FLATTOP:
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 1.0 - 1.93 * cos(2 * M_PI * windowPosition / windowEnd) +
                                                           1.29 * cos(4 * M_PI * windowPosition / windowEnd) -
                                                           0.388 * cos(6 * M_PI * windowPosition / windowEnd) +
                                                           0.032 * cos(8 * M_PI * windowPosition / windowEnd);
                break;
            default: // Dso::WINDOW_RECTANGULAR
                for (unsigned int windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                    *(lastWindowBuffer + windowPosition) = 1.0;
            }
        }

        // Set sampling interval
        channelData->spectrum.interval = 1.0 / channelData->voltage.interval / sampleCount;

        // Number of real/complex samples
        unsigned int dftLength = sampleCount / 2;

        // Reallocate memory for samples if the sample count has changed
        channelData->spectrum.sample.resize(sampleCount);

        // Create sample buffer and apply window
        std::unique_ptr<double[]> windowedValues = std::unique_ptr<double[]>(new double[sampleCount]);

        for (unsigned int position = 0; position < sampleCount; ++position)
            windowedValues[position] = lastWindowBuffer[position] * channelData->voltage.sample[position];

        {
            // Do discrete real to half-complex transformation
            /// \todo Check if record length is multiple of 2
            /// \todo Reuse plan and use FFTW_MEASURE to get fastest algorithm
            fftw_plan fftPlan = fftw_plan_r2r_1d(sampleCount, windowedValues.get(),
                                                 &channelData->spectrum.sample.front(), FFTW_R2HC, FFTW_ESTIMATE);
            fftw_execute(fftPlan);
            fftw_destroy_plan(fftPlan);
        }

        // Do an autocorrelation to get the frequency of the signal
        std::unique_ptr<double[]> conjugateComplex = std::move(windowedValues);

        // Real values
        unsigned int position;
        double correctionFactor = 1.0 / dftLength / dftLength;
        conjugateComplex[0] = (channelData->spectrum.sample[0] * channelData->spectrum.sample[0]) * correctionFactor;
        for (position = 1; position < dftLength; ++position)
            conjugateComplex[position] =
                (channelData->spectrum.sample[position] * channelData->spectrum.sample[position] +
                 channelData->spectrum.sample[sampleCount - position] *
                     channelData->spectrum.sample[sampleCount - position]) *
                correctionFactor;
        // Complex values, all zero for autocorrelation
        conjugateComplex[dftLength] =
            (channelData->spectrum.sample[dftLength] * channelData->spectrum.sample[dftLength]) * correctionFactor;
        for (++position; position < sampleCount; ++position) conjugateComplex[position] = 0;

        // Do half-complex to real inverse transformation
        std::unique_ptr<double[]> correlation = std::unique_ptr<double[]>(new double[sampleCount]);
        fftw_plan fftPlan =
            fftw_plan_r2r_1d(sampleCount, conjugateComplex.get(), correlation.get(), FFTW_HC2R, FFTW_ESTIMATE);
        fftw_execute(fftPlan);
        fftw_destroy_plan(fftPlan);

        // Calculate peak-to-peak voltage
        double minimalVoltage, maximalVoltage;
        minimalVoltage = maximalVoltage = channelData->voltage.sample[0];

        for (unsigned int position = 1; position < sampleCount; ++position) {
            if (channelData->voltage.sample[position] < minimalVoltage)
                minimalVoltage = channelData->voltage.sample[position];
            else if (channelData->voltage.sample[position] > maximalVoltage)
                maximalVoltage = channelData->voltage.sample[position];
        }

        channelData->amplitude = maximalVoltage - minimalVoltage;

        // Get the frequency from the correlation results
        double minimumCorrelation = correlation[0];
        double peakCorrelation = 0;
        unsigned int peakPosition = 0;

        for (unsigned int position = 1; position < sampleCount / 2; ++position) {
            if (correlation[position] > peakCorrelation && correlation[position] > minimumCorrelation * 2) {
                peakCorrelation = correlation[position];
                peakPosition = position;
            } else if (correlation[position] < minimumCorrelation)
                minimumCorrelation = correlation[position];
        }
        correlation.reset(nullptr);

        // Calculate the frequency in Hz
        if (peakPosition)
            channelData->frequency = 1.0 / (channelData->voltage.interval * peakPosition);
        else
            channelData->frequency = 0;

        // Finally calculate the real spectrum if we want it
        if (scope->spectrum[channel].used) {
            // Convert values into dB (Relative to the reference level)
            double offset = 60 - scope->spectrumReference - 20 * log10(dftLength);
            double offsetLimit = scope->spectrumLimit - scope->spectrumReference;
            for (std::vector<double>::iterator spectrumIterator = channelData->spectrum.sample.begin();
                 spectrumIterator != channelData->spectrum.sample.end(); ++spectrumIterator) {
                double value = 20 * log10(fabs(*spectrumIterator)) + offset;

                // Check if this value has to be limited
                if (offsetLimit > value) value = offsetLimit;

                *spectrumIterator = value;
            }
        }
    }
}
