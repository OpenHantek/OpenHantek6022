// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// \struct SampleValues                                          dataanalyzer.h
/// \brief Struct for a array of sample values.
struct SampleValues {
    std::vector<double> sample; ///< Vector holding the sampling data
    double interval = 0.0;      ///< The interval between two sample values
};

////////////////////////////////////////////////////////////////////////////////
/// \struct AnalyzedData                                          dataanalyzer.h
/// \brief Struct for the analyzed data.
struct DataChannel {
    SampleValues voltage;   ///< The time-domain voltage levels (V)
    SampleValues spectrum;  ///< The frequency-domain power levels (dB)
    double amplitude = 0.0; ///< The amplitude of the signal
    double frequency = 0.0; ///< The frequency of the signal
};

class DataAnalyzerResult {
  public:
    DataAnalyzerResult(unsigned int channelCount);
    const DataChannel *data(int channel) const;
    DataChannel *modifyData(int channel);
    unsigned int sampleCount() const;
    unsigned int channelCount() const;

    /**
     * Applies a new maximum samples value, if the given value is higher than the
     * already stored one
     * @param newMaxSamples Maximum samples value
     */
    void challengeMaxSamples(unsigned int newMaxSamples);
    unsigned int getMaxSamples() const;

  private:
    std::vector<DataChannel> analyzedData; ///< The analyzed data for each channel
    unsigned int maxSamples = 0;           ///< The maximum record length of the analyzed data
};
