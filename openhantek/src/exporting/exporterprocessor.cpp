// SPDX-License-Identifier: GPL-2.0-or-later

#include "exporterprocessor.h"
#include "exporterregistry.h"

ExporterProcessor::ExporterProcessor( ExporterRegistry *registry ) : registry( registry ) {}

void ExporterProcessor::process( PPresult *data ) { registry->addRawSamples( data ); }
