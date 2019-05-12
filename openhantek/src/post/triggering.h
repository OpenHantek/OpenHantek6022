// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "processor.h"

struct DsoSettingsScope;
class PPresult;

class Triggering : public Processor
{
public:
    Triggering(const DsoSettingsScope *scope, bool isSoftwareTriggerDevice);
    virtual ~Triggering();
    void process(PPresult *) override;
private:
    const DsoSettingsScope *scope;
    const bool isSoftwareTriggerDevice;
};
