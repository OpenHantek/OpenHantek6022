// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QLayout>

void registerDockMetaTypes();
void SetupDockWidget(QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout);
