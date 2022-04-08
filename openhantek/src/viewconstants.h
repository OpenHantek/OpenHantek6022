// SPDX-License-Identifier: GPL-2.0+

#pragma once

#define DIVS_TIME 10.0   ///< Number of horizontal screen divs
#define DIVS_VOLTAGE 8.0 ///< Number of vertical screen divs
#define DIVS_SUB 5       ///< Number of sub-divisions per div

#define MARGIN_LEFT ( -DIVS_TIME / 2.0 )
#define MARGIN_RIGHT ( DIVS_TIME / 2.0 )
#define MARGIN_TOP ( DIVS_VOLTAGE / 2.0 )
#define MARGIN_BOTTOM ( -DIVS_VOLTAGE / 2.0 )

#define MARKER_STEP ( DIVS_TIME / 100.0 )

// manual and modification docs
#define USER_MANUAL_NAME "OpenHantek6022_User_Manual.pdf"
#define AC_MODIFICATION_NAME "HANTEK6022_AC_Modification.pdf"
#define FREQUENCY_GENERATOR_MODIFICATION_NAME "HANTEK6022_Frequency_Generator_Modification.pdf"

// where are the (local) documents?
#ifdef Q_OS_WIN
#define DOC_PATH "./documents"
#else
#ifdef Q_OS_FREEBSD
#define DOC_PATH "/usr/local/share/doc/openhantek/"
#else
#define DOC_PATH "/usr/share/doc/openhantek/"
#endif
#endif

// GitHub doc location
#define DOC_URL "https://github.com/OpenHantek/OpenHantek6022/blob/main/docs/"
