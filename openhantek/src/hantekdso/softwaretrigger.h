#pragma once
#include <tuple>
#include "softwaretriggersettings.h"
struct DsoSettingsScope;
class DataAnalyzerResult;

class SoftwareTrigger
{
public:
    typedef std::tuple<unsigned, unsigned, unsigned> PrePostStartTriggerSamples;
    static PrePostStartTriggerSamples computeTY(const DataAnalyzerResult *result,
                                                               const DsoSettingsScope *scope,
                                                               unsigned physicalChannels);
};
