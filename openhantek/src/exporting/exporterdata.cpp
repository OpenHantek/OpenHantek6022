// SPDX-License-Identifier: GPL-2.0-or-later

#include "exporterdata.h"

ExporterData::ExporterData( const std::shared_ptr< PPresult > &data, const DsoSettingsScope &scope ) {
    _isSpectrumUsed = false;
    _timeInterval = 0;
    _freqInterval = 0;
    _maxRow = 0;
    _chCount = scope.voltage.size();
    _voltageData = std::vector< const SampleValues * >( size_t( _chCount ), nullptr );
    _spectrumData = std::vector< const SampleValues * >( size_t( _chCount ), nullptr );

    for ( ChannelID channel = 0; channel < _chCount; ++channel ) {
        if ( data->data( channel ) ) {
            if ( scope.voltage[ channel ].used ) {
                _voltageData[ channel ] = &( data->data( channel )->voltage );
                _maxRow = qMax( _maxRow, _voltageData[ channel ]->samples.size() );
                _timeInterval = data->data( channel )->voltage.interval;
            }
            if ( scope.spectrum[ channel ].used ) {
                _spectrumData[ channel ] = &( data->data( channel )->spectrum );
                _maxRow = qMax( _maxRow, _spectrumData[ channel ]->samples.size() );
                _freqInterval = data->data( channel )->spectrum.interval;
                _isSpectrumUsed = true;
            }
        }
    }
}
