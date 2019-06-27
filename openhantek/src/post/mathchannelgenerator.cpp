#include "mathchannelgenerator.h"
#include "scopesettings.h"
#include "post/postprocessingsettings.h"
#include "enums.h"

MathChannelGenerator::MathChannelGenerator(const DsoSettingsScope *scope, unsigned physicalChannels)
    : physicalChannels(physicalChannels), scope(scope) {}


MathChannelGenerator::~MathChannelGenerator() {}


void MathChannelGenerator::process(PPresult *result) {
    //printf( "MathChannelGenerator::process\n" );
    // Math channel enabled?
    if (!scope->voltage[physicalChannels].used && !scope->spectrum[physicalChannels].used)
        return;

    DataChannel *const channelData = result->modifyData(physicalChannels);
    std::vector<double> &resultData = channelData->voltage.sample;

    unsigned src = 0;

    if ( Dso::getMathMode( scope->voltage[physicalChannels] ) < Dso::MathMode::AC_CH1 ) { // binary operations
        if ( result->data(0)->voltage.sample.empty() || result->data(1)->voltage.sample.empty() )
            return;
        // Resize the sample vector
        resultData.resize(std::min(result->data(0)->voltage.sample.size(), result->data(1)->voltage.sample.size()));
        // Set sampling interval
        channelData->voltage.interval = result->data(0)->voltage.interval;
        // Calculate values and write them into the sample buffer
        std::vector<double>::const_iterator ch1Iterator = result->data(0)->voltage.sample.begin();
        std::vector<double>::const_iterator ch2Iterator = result->data(1)->voltage.sample.begin();
        for (std::vector<double>::iterator it = resultData.begin(); it != resultData.end(); ++it) {
            switch (Dso::getMathMode(scope->voltage[physicalChannels])) {
                case Dso::MathMode::ADD_CH1_CH2:
                    *it = *ch1Iterator + *ch2Iterator;
                    break;
                case Dso::MathMode::SUB_CH2_FROM_CH1:
                    *it = *ch1Iterator - *ch2Iterator;
                    break;
                case Dso::MathMode::SUB_CH1_FROM_CH2:
                    *it = *ch2Iterator - *ch1Iterator;
                    break;
                default:
                    break;
            }
            ++ch1Iterator;
            ++ch2Iterator;
        }
    } else { // unary operators (calculate "AC coupling")
        if ( Dso::getMathMode( scope->voltage[physicalChannels] ) == Dso::MathMode::AC_CH1 )
            src = 0;
        else if ( Dso::getMathMode( scope->voltage[physicalChannels] ) == Dso::MathMode::AC_CH2 )
            src = 1;

        // Resize the sample vector
        resultData.resize( result->data( src )->voltage.sample.size() );
        // Set sampling interval
        channelData->voltage.interval = result->data( src )->voltage.interval;

        // calculate DC component of channel...
        double average = 0;
        for ( std::vector<double>::const_iterator srcIt = result->data( src )->voltage.sample.begin();
              srcIt != result->data( src )->voltage.sample.end(); srcIt++ ) {
            average += *srcIt;
        }
        average /= result->data( src )->voltage.sample.size();

        // ... and remove DC component to get AC
        std::vector<double>::const_iterator srcIt = result->data( src )->voltage.sample.begin();
        for ( std::vector<double>::iterator dstIt = resultData.begin(); dstIt != resultData.end(); ++dstIt ) {
            *dstIt = *srcIt++ - average;
        }
    }
}
