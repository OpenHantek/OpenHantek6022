// SPDX-License-Identifier: GPL-2.0-or-later

#include "controlspecification.h"

Dso::ControlSpecification::ControlSpecification( unsigned channels ) : channels( channels ) { voltageScale.resize( channels ); }
