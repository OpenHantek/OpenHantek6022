// SPDX-License-Identifier: GPL-2.0+

#include "dataanalyzerresult.h"
#include <QDebug>
#include <stdexcept>

DataAnalyzerResult::DataAnalyzerResult(unsigned int channelCount) { analyzedData.resize(channelCount); }

/// \brief Returns the analyzed data.
/// \param channel Channel, whose data should be returned.
/// \return Analyzed data as AnalyzedData struct.
const DataChannel *DataAnalyzerResult::data(ChannelID channel) const {
    if (channel >= this->analyzedData.size()) return 0;

    return &this->analyzedData[(size_t)channel];
}

DataChannel *DataAnalyzerResult::modifyData(ChannelID channel) {
    if (channel >= this->analyzedData.size())
        throw new std::runtime_error("If you modfiy the DataAnalyzerResult, you "
                                     "need to set the channels first!");

    return &this->analyzedData[(size_t)channel];
}

/// \brief Returns the sample count of the analyzed data.
/// \return The maximum sample count of the last analyzed data.
unsigned int DataAnalyzerResult::sampleCount() const { return this->maxSamples; }

unsigned int DataAnalyzerResult::channelCount() const { return analyzedData.size(); }

void DataAnalyzerResult::challengeMaxSamples(unsigned int newMaxSamples) {
    if (newMaxSamples > this->maxSamples) this->maxSamples = newMaxSamples;
}

unsigned int DataAnalyzerResult::getMaxSamples() const { return maxSamples; }
