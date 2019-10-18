// SPDX-License-Identifier: GPL-2.0+

#pragma once

#define SAMPLESIZE_USED 20000

#define DIVS_TIME 10.0   ///< Number of horizontal screen divs
#define DIVS_VOLTAGE 8.0 ///< Number of vertical screen divs
#define DIVS_SUB 5       ///< Number of sub-divisions per div

#define MARGIN_LEFT (-DIVS_TIME / 2.0)
#define MARGIN_RIGHT (DIVS_TIME / 2.0)

#define MARKER_COUNT 2 ///< Number of markers
#define MARKER_STEP (DIVS_TIME / 100.0)

// spacing between the individual entries of the docks
#define DOCK_LAYOUT_SPACING 4
