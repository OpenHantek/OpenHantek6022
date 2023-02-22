// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QReadWriteLock>
#include <QVector3D>

#include "hantekprotocol/types.h"
#include "utils/printutils.h"
#include <vector>

/// \brief Struct for a array of sample values.
struct SampleValues {
    std::vector< double > samples; ///< Vector holding the sampling data
    double interval = 0.0;         ///< The interval between two sample values
};

/// \brief Struct for the analyzed data.
struct DataChannel {
    SampleValues voltage;          ///< The time-domain voltage levels (V)
    SampleValues spectrum;         ///< The frequency-domain power levels (dB)
    bool valid = true;             ///< Not clipped, distorted, dropouts etc.
    double vmin = 0.0;             ///< The minimum sample value of _displayed_ part of trace
    double vmax = 0.0;             ///< The maximum sample value of _displayed_ part of trace
    double rms = 0.0;              ///< The DC + AC rms value of the signal = sqrt( dc * dc + acc * ac )
    double dBmin = 0.0;            ///< The minimum magnitude value
    double dBmax = 0.0;            ///< The maximum magnitude value
    double dc = 0.0;               ///< The DC bias of the signal
    double ac = 0.0;               ///< The AC rms value of the signal
    double dB = 0.0;               ///< The AC rms value as dB (dBV or other depending on config)
    double frequency = 0.0;        ///< The frequency of the signal
    QString note = "";             ///< The note value of the frequency
    double thd = 0.0;              ///< The THD value
    double pulseWidth1 = 0.0;      ///< The width of the triggered pulse
    double pulseWidth2 = 0.0;      ///< The width of the following pulse
    Unit voltageUnit = UNIT_VOLTS; ///< unless UNIT_VOLTSQUARE for some math functions
};

typedef std::vector< QVector3D > ChannelGraph;
typedef std::vector< ChannelGraph > ChannelsGraphs;

/// Post processing results
class PPresult {
  public:
    explicit PPresult( unsigned int channelCount );

    /// \brief Returns the analyzed data (RO).
    /// \param channel Channel, whose data should be returned.
    const DataChannel *data( ChannelID channel ) const;
    /// \brief Returns the analyzed data (RW). The data structure can be modified.
    /// \param channel Channel, whose data should be returned.
    DataChannel *modifiableData( ChannelID channel );
    /// \return The maximum sample count of the last analyzed data. This assumes there is at least one channel.
    unsigned int sampleCount() const;
    unsigned int channelCount() const;

    /// sw trigger status
    bool softwareTriggerTriggered = false;
    /// skip samples at start of channel to get triggered trace on screen
    int triggeredPosition = 0; ///< Not triggered
    double pulseWidth1 = 0.0;  ///< The width of the triggered pulse
    double pulseWidth2 = 0.0;  ///< The width of the following pulse
    unsigned tag;              ///< track individual sample blocks (debug support)

    ChannelsGraphs vaChannelSpectrum;
    ChannelsGraphs vaChannelVoltage;
    ChannelsGraphs vaChannelHistogram;

  private:
    std::vector< DataChannel > analyzedData; ///< The analyzed data for each channel
};
