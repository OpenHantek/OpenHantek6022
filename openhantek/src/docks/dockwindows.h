// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDockWidget>
#include <QLayout>

#include "dockconstants.h"
#include "viewconstants.h"


void registerDockMetaTypes();
void SetupDockWidget( QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout );
