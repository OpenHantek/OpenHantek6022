// SPDX-License-Identifier: GPL-2.0+

#include "ppresult.h"
#include <QDebug>
#include <stdexcept>

PPresult::PPresult(unsigned int channelCount) { analyzedData.resize(channelCount); }

const DataChannel *PPresult::data(ChannelID channel) const {
    if (channel >= this->analyzedData.size())
        return nullptr;
    return &this->analyzedData[channel];
}

DataChannel *PPresult::modifyData(ChannelID channel) { return &this->analyzedData[channel]; }

unsigned int PPresult::sampleCount() const { return unsigned(analyzedData[0].voltage.sample.size()); }

unsigned int PPresult::channelCount() const { return unsigned(analyzedData.size()); }
