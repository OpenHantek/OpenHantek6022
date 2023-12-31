// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

// manual and modification docs
const QString UserManualName( "OpenHantek6022_User_Manual.pdf" );
const QString ACModificationName( "HANTEK6022_AC_Modification.pdf" );
const QString FrequencyGeneratorModificationName( "HANTEK6022_Frequency_Generator_Modification.pdf" );

// where are the (local) documents?
#if defined( Q_OS_WIN )
const QString DocPath( "documents\\" );
#elif defined( Q_OS_FREEBSD )
const QString DocPath( "/usr/local/share/doc/openhantek/" );
#else
const QString DocPath( "/usr/share/doc/openhantek/" );
#endif

// GitHub doc location
const QString DocUrl( "https://github.com/OpenHantek/OpenHantek6022/blob/main/docs/" );
