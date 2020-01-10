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
    bool valid = true;      ///< Not clipped, distorted, dropouts etc.
    double vpp = 0.0;       ///< The peak-to-peak voltage of the _displayed_ part of trace
    double rms = 0.0;       ///< The DC + AC rms value of the signal = sqrt( dc * dc + acc * ac )
    double dc = 0.0;        ///< The DC bias of the signal
    double ac = 0.0;        ///< The AC rms value of the signal
    double dB = 0.0;        ///< The AC rms value as dB (dBV or other depending on config)
    double frequency = 0.0; ///< The frequency of the signal
    double pulseWidth1 = 0.0;///< The width of the triggered pulse
    double pulseWidth2 = 0.0;///< The width of the following pulse
};

typedef std::vector<QVector3D> ChannelGraph;
typedef std::vector<ChannelGraph> ChannelsGraphs;

/// Post processing results
class PPresult {
  public:
    explicit PPresult(unsigned int channelCount);

    /// \brief Returns the analyzed data.
    /// \param channel Channel, whose data should be returned.
    const DataChannel *data(ChannelID channel) const;
    /// \brief Returns the analyzed data. The data structure can be modifed.
    /// \param channel Channel, whose data should be returned.
    DataChannel *modifyData(ChannelID channel);
    /// \return The maximum sample count of the last analyzed data. This assumes there is at least one channel.
    unsigned int sampleCount() const;
    unsigned int channelCount() const;

    /// sw trigger status
    bool softwareTriggerTriggered = false;
    /// skip samples at start of channel to get triggered trace on screen
    unsigned triggeredPosition = 0;///< Not triggered
    double pulseWidth1 = 0.0;///< The width of the triggered pulse
    double pulseWidth2 = 0.0;///< The width of the following pulse

    ChannelsGraphs vaChannelSpectrum;
    ChannelsGraphs vaChannelVoltage;

  private:
    std::vector<DataChannel> analyzedData; ///< The analyzed data for each channel
};
