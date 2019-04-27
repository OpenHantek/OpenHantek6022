#pragma once

#include "post/processor.h"

class ExporterRegistry;

class ExporterProcessor : public Processor
{
public:
    ExporterProcessor(ExporterRegistry* registry);
    void process(PPresult *) override;
private:
    ExporterRegistry* registry;
};
