#pragma once
#include <tuple>
struct DsoSettingsScope;
class DataAnalyzerResult;


/**
 * Contains software trigger algorithms. At the moment this works on the analysed data of the
 * DataAnalyser class.
 * TODO Should work on the raw data within HantekDsoControl
 */
class SoftwareTrigger {
  public:
    typedef std::tuple<unsigned, unsigned, unsigned> PrePostStartTriggerSamples;
    /**
     * @brief Computes a software trigger point.
     * @param data Analysed data from the
     * @param scope Scope settings
     * @return Returns a tuple of positions [preTrigger, postTrigger, startTrigger]
     */
    static PrePostStartTriggerSamples compute(const DataAnalyzerResult *data, const DsoSettingsScope *scope);
};
