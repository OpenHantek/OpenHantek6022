// SPDX-License-Identifier: GPL-2.0+

#include "postprocessing.h"

PostProcessing::PostProcessing( unsigned channelCount ) : channelCount( channelCount ) {
    qRegisterMetaType< std::shared_ptr< PPresult > >();
}

void PostProcessing::registerProcessor( Processor *processor ) { processors.push_back( processor ); }

void PostProcessing::convertData( const DSOsamples *source, PPresult *destination ) {
    // printf( "PostProcessing::convertData()\n" );
    QReadLocker locker( &source->lock );
    if ( source->triggerPosition ) {
        destination->softwareTriggerTriggered = source->liveTrigger;
        destination->triggeredPosition = source->triggerPosition;
        destination->pulseWidth1 = source->pulseWidth1;
        destination->pulseWidth2 = source->pulseWidth2;
    } else {
        destination->softwareTriggerTriggered = false;
        destination->triggeredPosition = 0;
        destination->pulseWidth1 = 0;
        destination->pulseWidth2 = 0;
    }

    for ( ChannelID channel = 0; channel < source->data.size(); ++channel ) {
        const std::vector< double > &rawChannelData = source->data.at( channel );

        if ( rawChannelData.empty() ) {
            continue;
        }
        DataChannel *const channelData = destination->modifiableData( channel );
        channelData->voltage.interval = 1.0 / source->samplerate;
        channelData->voltage.sample = rawChannelData;
        // printf( "PP CH%d: %d\n", channel+1, source->clipped );
        channelData->valid = !( source->clipped & ( 0x01 << channel ) );
    }
    destination->tag = source->tag;
}

void PostProcessing::input( const DSOsamples *data ) {
    // printf( "PostProcessing::input()\n" );
    currentData.reset( new PPresult( channelCount ) ); // start with a fresh data structure
    convertData( data, currentData.get() );            // copy all relevant data over
    for ( Processor *p : processors )                  // feed it into the PP chain
        p->process( currentData.get() );
    std::shared_ptr< PPresult > res = std::move( currentData );
    emit processingFinished( res );
}
