// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QVector3D>
#include <QReadWriteLock>

#include <vector>
#include "hantekprotocol/types.h"

/// \brief Struct for a array of sample values.
struct SampleValues {
    std::vector<double> sample; ///< Vector holding the sampling data
    double interval = 0.0;      ///< The interval between two sample values
};

/// \brief Struct for the analyzed data.
struct DataChannel {
    SampleValues voltage;   ///< The time-domain voltage levels (V)
    SampleValues spectrum;  ///< The frequency-domain power levels (dB)

    double frequency = 0.0; ///< The frequency of the signal
    // Calculate peak-to-peak voltage
    double computeAmplitude() const;
};

typedef std::vector<QVector3D> ChannelGraph;
typedef std::vector<ChannelGraph> ChannelsGraphs;

/// Post processing results
class PPresult {
  public:
    PPresult(unsigned int channelCount);

    /// \brief Returns the analyzed data.
    /// \param channel Channel, whose data should be returned.
    const DataChannel *data(ChannelID channel) const;
    /// \brief Returns the analyzed data. The data structure can be modifed.
    /// \param channel Channel, whose data should be returned.
    DataChannel *modifyData(ChannelID channel);
    /// \return The maximum sample count of the last analyzed data. This assumes there is at least one channel.
    unsigned int sampleCount() const;
    unsigned int channelCount() const;

    bool softwareTriggerTriggered = false;

    ChannelsGraphs vaChannelSpectrum;
    ChannelsGraphs vaChannelVoltage;
  private:
    std::vector<DataChannel> analyzedData; ///< The analyzed data for each channel
};
