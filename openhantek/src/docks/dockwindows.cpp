// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "dockwindows.h"
#include "hantekdso/enums.h"
#include "hantekdso/mathmodes.h"
#include "hantekprotocol/types.h"
#include "post/analysissettings.h"


void SetupDockWidget( QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout ) {
    dockWindow->setObjectName( dockWindow->windowTitle() );
    dockWindow->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    dockWindow->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable ); // do not close
    dockWidget->setLayout( layout );
    dockWidget->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::DefaultType ) );
    dockWindow->setWidget( dockWidget );
}


void registerDockMetaTypes() {
    qRegisterMetaType< Dso::TriggerMode >();
    qRegisterMetaType< Dso::MathMode >();
    qRegisterMetaType< Dso::Slope >();
    qRegisterMetaType< Dso::Coupling >();
    qRegisterMetaType< Dso::GraphFormat >();
    qRegisterMetaType< Dso::ChannelMode >();
    qRegisterMetaType< Dso::WindowFunction >();
    qRegisterMetaType< Dso::InterpolationMode >();
    qRegisterMetaType< std::vector< unsigned > >();
    qRegisterMetaType< std::vector< double > >();
    qRegisterMetaType< ChannelID >( "ChannelID" );
}
