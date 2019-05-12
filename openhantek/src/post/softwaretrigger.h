#pragma once
#include <tuple>
struct DsoSettingsScope;
class PPresult;


/**
 * Contains software trigger algorithms. At the moment this works on the analysed data of the
 * DataAnalyser class.
 * TODO Should work on the raw data within HantekDsoControl
 */
class SoftwareTrigger {
  public:
    typedef std::pair<bool, unsigned> TriggerStatusSkip;
    /**
     * @brief Computes a software trigger point.
     * @param data Analysed data from the
     * @param scope Scope settings
     * @return Returns a tuple of positions [preTrigger, postTrigger, startTrigger]
     */
    static TriggerStatusSkip compute(const PPresult *data, const DsoSettingsScope *scope);
};
