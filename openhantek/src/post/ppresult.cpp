// SPDX-License-Identifier: GPL-2.0+

#include "ppresult.h"
#include <QDebug>

PPresult::PPresult( unsigned int channelCount ) { analyzedData.resize( channelCount ); }

const DataChannel *PPresult::data( ChannelID channel ) const {
    if ( channel >= analyzedData.size() )
        return nullptr;
    return &analyzedData[ channel ];
}

DataChannel *PPresult::modifiableData( ChannelID channel ) { return &analyzedData[ channel ]; }

unsigned int PPresult::sampleCount() const { return unsigned( analyzedData[ 0 ].voltage.sample.size() ); }

unsigned int PPresult::channelCount() const { return unsigned( analyzedData.size() ); }
