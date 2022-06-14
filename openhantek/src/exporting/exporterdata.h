// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "exporterregistry.h"
#include "post/ppresult.h"
#include "scopesettings.h"

#include <memory>
#include <vector>

class ExporterData {
  public:
    ExporterData( const std::shared_ptr< PPresult > &data, const DsoSettingsScope &scope );

    const size_t &getChannelsCount() const { return _chCount; }
    const size_t &getMaxRow() const { return _maxRow; }
    const bool &isSpectrumUsed() const { return _isSpectrumUsed; }
    const double &getTimeInterval() const { return _timeInterval; }
    const double &getFreqInterval() const { return _freqInterval; }
    std::vector< const SampleValues * > const &getVoltageData() const { return _voltageData; }
    std::vector< const SampleValues * > const &getSpectrumData() const { return _spectrumData; }

  private:
    size_t _chCount;
    size_t _maxRow;
    bool _isSpectrumUsed;
    double _timeInterval;
    double _freqInterval;
    std::vector< const SampleValues * > _voltageData;
    std::vector< const SampleValues * > _spectrumData;
};
