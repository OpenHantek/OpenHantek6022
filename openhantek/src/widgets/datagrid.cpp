// SPDX-License-Identifier: GPL-2.0-or-later

#include "datagrid.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

DataGrid::DataGrid( QWidget *parent ) : QGroupBox( parent ) {
    cursorsLayout = new QGridLayout();
    cursorsLayout->setSpacing( 5 );
    cursorsSelectorGroup = new QButtonGroup();
    cursorsSelectorGroup->setExclusive( true );

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0 ) )
    connect( cursorsSelectorGroup, static_cast< void ( QButtonGroup::* )( int ) >( &QButtonGroup::idPressed ),
#else
    connect( cursorsSelectorGroup, static_cast< void ( QButtonGroup::* )( int ) >( &QButtonGroup::buttonPressed ),
#endif
             [ this ]( int index ) { emit itemSelected( index ); } );

    setLayout( cursorsLayout );
    setFixedWidth( 150 ); // do not waste too much screen space
}


DataGrid::CursorInfo::CursorInfo() {
    selector = new QPushButton();
    selector->setCheckable( true );
    onOff = new QPushButton();
    deltaXLabel = new QLabel();
    deltaXLabel->setAlignment( Qt::AlignRight );
    deltaYLabel = new QLabel();
    deltaYLabel->setAlignment( Qt::AlignRight );
}


void DataGrid::CursorInfo::configure( const QString &text, const QColor &bgColor, const QColor &fgColor ) {
    palette.setColor( QPalette::Window, bgColor );
    palette.setColor( QPalette::WindowText, fgColor );

    selector->setText( text );
    selector->setStyleSheet(
        QString( R"(
        QPushButton {
            color: %2;
            background-color: %1;
            border: 1px solid %2;
        }
        QPushButton:checked {
            color: %1;
            background-color: %2;
            border: 1px solid %1;
        }
        QPushButton:disabled {
            color: %3;
            background-color: %1;
            border: 1px dotted %2;
        }
    )" )
            .arg( bgColor.name( QColor::HexArgb ), fgColor.name( QColor::HexArgb ), fgColor.darker().name( QColor::HexArgb ) ) );

    onOff->setStyleSheet(
        QString( R"(
        QPushButton {
            color: %2;
            background-color: %1;
            border: 1px solid %2;
        }
        QPushButton:checked {
            color: %1;
            background-color: %2;
        }
        QPushButton:disabled {
            color: %3;
            border: none;
        }
    )" )
            .arg( bgColor.name( QColor::HexArgb ), fgColor.name( QColor::HexArgb ), fgColor.darker().name( QColor::HexArgb ) ) );

    deltaXLabel->setPalette( palette );
    deltaYLabel->setPalette( palette );
}


void DataGrid::setBackgroundColor( const QColor &bgColor ) {
    backgroundColor = bgColor;
    for ( auto it : items ) {
        it.configure( it.selector->text(), bgColor, it.palette.color( QPalette::WindowText ) );
    }
    setStyleSheet( "DataGrid {background-color: " + backgroundColor.name() + " }" );
}


void DataGrid::configureItem( unsigned index, const QColor &fgColor ) {
    if ( index < items.size() ) {
        items[ index ].configure( items[ index ].selector->text(), backgroundColor, fgColor );
    }
}


int DataGrid::addItem( const QString &text, const QColor &fgColor ) {
    int index = int( items.size() );
    items.resize( ulong( index + 1 ) );

    CursorInfo &info = items.at( ulong( index ) );
    info.configure( text, backgroundColor, fgColor );
    cursorsSelectorGroup->addButton( info.selector, index );

    connect( info.onOff, &QPushButton::clicked, [ this, index ]() { emit itemUpdated( index ); } );

    cursorsLayout->addWidget( info.selector, 3 * index, 0 );
    if ( index ) // no ON/OFF button for markers
        cursorsLayout->addWidget( info.onOff, 3 * index, 1 );
    cursorsLayout->addWidget( info.deltaXLabel, 3 * index + 1, 0 );
    cursorsLayout->addWidget( info.deltaYLabel, 3 * index + 1, 1 );
    cursorsLayout->setRowMinimumHeight( 3 * index + 2, 10 );
    cursorsLayout->setRowStretch( 3 * index, 0 );
    cursorsLayout->setRowStretch( 3 * index + 3, 1 );

    return index;
}


void DataGrid::updateInfo( unsigned index, bool visible, const QString &strOnOff, const QString &strX, const QString &strY ) {
    if ( index >= items.size() )
        return;
    CursorInfo &info = items.at( index );
    info.selector->setEnabled( visible );
    info.onOff->setEnabled( visible );
    if ( visible ) {
        if ( toolTipsVisible ) {
            info.selector->setToolTip( tr( "Select the active cursor" ) );
            info.onOff->setToolTip( tr( "Show cursor and measurement" ) );
        }
        info.onOff->setText( strOnOff );
        info.deltaXLabel->setText( strX );
        info.deltaYLabel->setText( strY );
    } else {
        info.selector->setToolTip( QString() );
        info.onOff->setToolTip( QString() );
        info.onOff->setText( QString() );
        info.deltaXLabel->setText( QString() );
        info.deltaYLabel->setText( QString() );
    }
}


void DataGrid::selectItem( unsigned index ) {
    if ( index >= items.size() )
        return;
    items[ index ].selector->setChecked( true );
}


void DataGrid::setToolTipsVisible( bool visible ) { toolTipsVisible = visible; }
