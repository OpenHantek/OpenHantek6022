#include "exporterprocessor.h"
#include "exporterregistry.h"

ExporterProcessor::ExporterProcessor(ExporterRegistry *registry) : registry(registry) {}

void ExporterProcessor::process(PPresult *data) { registry->addRawSamples(data); }
