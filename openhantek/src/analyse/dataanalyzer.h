// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

#include <QMutex>
#include <QThread>
#include <memory>

#include "dataanalyzerresult.h"
#include "definitions.h"
#include "dsosamples.h"
#include "utils/printutils.h"

struct DsoSettingsScope;

////////////////////////////////////////////////////////////////////////////////
/// \class DataAnalyzer                                           dataanalyzer.h
/// \brief Analyzes the data from the dso.
/// Calculates the spectrum and various data about the signal and saves the
/// time-/frequencysteps between two values.
class DataAnalyzer : public QObject {
    Q_OBJECT

  public:
    ~DataAnalyzer();
    void applySettings(DsoSettingsScope *scope);
    void setSourceData(const DSOsamples *data);
    std::unique_ptr<DataAnalyzerResult> getNextResult();
    /**
     * Call this if the source data changed.
     */
    void samplesAvailable();

  private:
    static std::unique_ptr<DataAnalyzerResult> convertData(const DSOsamples *data, const DsoSettingsScope *scope);
    static void spectrumAnalysis(DataAnalyzerResult *result, Dso::WindowFunction &lastWindow,
                                 unsigned int lastRecordLength, double *&lastWindowBuffer,
                                 const DsoSettingsScope *scope);

  private:
    DsoSettingsScope *scope;
    unsigned int lastRecordLength = 0;                        ///< The record length of the previously analyzed data
    Dso::WindowFunction lastWindow = (Dso::WindowFunction)-1; ///< The previously used dft window function
    double *window = nullptr;
    const DSOsamples *sourceData = nullptr;
    std::unique_ptr<DataAnalyzerResult> lastResult;
  signals:
    void analyzed();
};
