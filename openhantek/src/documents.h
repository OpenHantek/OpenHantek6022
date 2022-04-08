// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QString>

// manual and modification docs
const QString UserManualName = QString( "OpenHantek6022_User_Manual.pdf" );
const QString ACModificationName = QString( "HANTEK6022_AC_Modification.pdf" );
const QString FrequencyGeneratorModificationName = QString( "HANTEK6022_Frequency_Generator_Modification.pdf" );

// where are the (local) documents?
#ifndef Q_OS_WIN
#ifdef Q_OS_FREEBSD
const QString DocPath = QString( "/usr/local/share/doc/openhantek/" );
#else
const QString DocPath = QString( "/usr/share/doc/openhantek/" );
#endif
#endif

// GitHub doc location
const QString DocUrl = QString( "https://github.com/OpenHantek/OpenHantek6022/blob/main/docs/" );
