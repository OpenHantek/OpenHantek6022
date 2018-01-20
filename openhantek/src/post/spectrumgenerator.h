// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

#include <QMutex>
#include <QThread>
#include <memory>

#include "ppresult.h"
#include "dsosamples.h"
#include "utils/printutils.h"
#include "postprocessingsettings.h"

#include "processor.h"

class DsoSettings;
struct DsoSettingsScope;

/// \brief Analyzes the data from the dso.
/// Calculates the spectrum and various data about the signal and saves the
/// time-/frequencysteps between two values.
class SpectrumGenerator : public Processor {
  public:
    SpectrumGenerator(const DsoSettingsScope* scope, const DsoSettingsPostProcessing* postprocessing);
    virtual ~SpectrumGenerator();
    virtual void process(PPresult *data) override;

  private:
    const DsoSettingsScope* scope;
    const DsoSettingsPostProcessing* postprocessing;
    unsigned int lastRecordLength = 0;                        ///< The record length of the previously analyzed data
    Dso::WindowFunction lastWindow = (Dso::WindowFunction)-1; ///< The previously used dft window function
    double *lastWindowBuffer = nullptr;
};
