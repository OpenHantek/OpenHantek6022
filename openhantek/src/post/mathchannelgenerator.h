// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "processor.h"

struct DsoSettingsScope;
class PPresult;

class MathChannelGenerator : public Processor
{
public:
    MathChannelGenerator(const DsoSettingsScope *scope, unsigned physicalChannels);
    virtual ~MathChannelGenerator();
    virtual void process(PPresult *) override;
private:
    const unsigned physicalChannels;
    const DsoSettingsScope *scope;
};
