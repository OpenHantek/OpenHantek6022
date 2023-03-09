// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>

#include "dsowidget.h"

#include "hantekdso/mathmodes.h"

#include "glscope.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "widgets/datagrid.h"
#include "widgets/levelslider.h"


DsoWidget::DsoWidget( DsoSettingsScope *scope, DsoSettingsView *view, const Dso::ControlSpecification *spec, QWidget *parent )
    : QWidget( parent ), scope( scope ), view( view ), spec( spec ), mainScope( GlScope::createNormal( scope, view ) ),
      zoomScope( GlScope::createZoomed( scope, view ) ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " DsoWidget::DsoWidget()";

    // get the primary screen size for further use - e.g. graphgenerator.cpp
    QSize screenSize = QGuiApplication::primaryScreen()->size();
    view->screenHeight = unsigned( screenSize.height() );
    view->screenWidth = unsigned( screenSize.width() );

    // Palette for this widget
    QPalette palette;
    palette.setColor( QPalette::Window, view->colors->background );
    palette.setColor( QPalette::WindowText, view->colors->text );

    setupSliders( mainSliders );
    setupSliders( zoomSliders );

    // movement of the two vertical markers "1" and "2"
    connect( mainScope, &GlScope::markerMoved, mainScope, [ this ]( int cursorIndex, int marker ) {
        mainSliders.markerSlider->setValue( marker, this->scope->getMarker( marker ) );
        mainScope->updateCursor( cursorIndex );
        zoomScope->updateCursor( cursorIndex );
    } );
    connect( zoomScope, &GlScope::markerMoved, mainScope, [ this ]( int cursorIndex, int marker ) {
        mainSliders.markerSlider->setValue( int( marker ), this->scope->getMarker( marker ) );
        mainScope->updateCursor( cursorIndex );
        zoomScope->updateCursor( cursorIndex );
    } );

    // do cursor measurement when right button pressed/moved _inside_ window borders
    connect( mainScope, &GlScope::cursorMeasurement, this, [ this ]( QPointF mPos, QPoint gPos, bool status ) {
        cursorMeasurementPosition = mPos;
        cursorGlobalPosition = gPos;
        cursorMeasurementValid = status;
        if ( !status )
            showCursorMessage(); // switch off
    } );
    connect( zoomScope, &GlScope::cursorMeasurement, this, [ this ]( QPointF mPos, QPoint gPos, bool status ) {
        cursorMeasurementPosition = mPos;
        cursorGlobalPosition = gPos;
        cursorMeasurementValid = status;
        if ( !status )
            showCursorMessage(); // switch off
    } );

    // The table for the settings at screen top
    settingsTriggerLabel = new QLabel();
    settingsTriggerLabel->setMinimumWidth( 320 );
    settingsTriggerLabel->setIndent( 5 );
    settingsSamplesOnScreen = new QLabel();
    settingsSamplesOnScreen->setPalette( palette );
    settingsSamplerateLabel = new QLabel();
    settingsSamplerateLabel->setPalette( palette );
    settingsTimebaseLabel = new QLabel();
    settingsTimebaseLabel->setPalette( palette );
    settingsFrequencybaseLabel = new QLabel();
    settingsFrequencybaseLabel->setPalette( palette );
    swTriggerStatus = new QLabel();
    swTriggerStatus->setMinimumWidth( 20 );
    swTriggerStatus->setAutoFillBackground( true );
    swTriggerStatus->setVisible( false );
    settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget( swTriggerStatus, 0, Qt::AlignCenter );
    settingsLayout->addWidget( settingsTriggerLabel );
    settingsLayout->addWidget( settingsSamplesOnScreen, 1, Qt::AlignRight );
    settingsLayout->addWidget( settingsSamplerateLabel, 1, Qt::AlignRight );
    settingsLayout->addWidget( settingsTimebaseLabel, 1, Qt::AlignRight );
    settingsLayout->addWidget( settingsFrequencybaseLabel, 1, Qt::AlignRight );

    // The table for the marker details
    markerInfoLabel = new QLabel();
    markerInfoLabel->setPalette( palette );
    markerTimeLabel = new QLabel();
    markerTimeLabel->setPalette( palette );
    markerFrequencyLabel = new QLabel();
    markerFrequencyLabel->setPalette( palette );
    markerTimebaseLabel = new QLabel();
    markerTimebaseLabel->setPalette( palette );
    markerFrequencybaseLabel = new QLabel();
    markerFrequencybaseLabel->setPalette( palette );
    markerLayout = new QHBoxLayout();
    markerLayout->addWidget( markerInfoLabel, 0, Qt::AlignLeft );
    markerLayout->addWidget( markerTimeLabel, 1, Qt::AlignLeft );
    markerLayout->addWidget( markerFrequencyLabel, 1, Qt::AlignLeft );
    markerLayout->addWidget( markerTimebaseLabel, 1, Qt::AlignRight );
    markerLayout->addWidget( markerFrequencybaseLabel, 1, Qt::AlignRight );

    // The table for the measurements at screen bottom
    measurementLayout = new QGridLayout();
    measurementLayout->setColumnMinimumWidth( 0, 50 ); // Channel
    measurementLayout->setColumnMinimumWidth( 1, 30 ); // Coupling or Mode
    measurementLayout->setColumnStretch( 2, 3 );       // Voltage amplitude
    measurementLayout->setColumnStretch( 3, 3 );       // Spectrum magnitude
    measurementLayout->setColumnStretch( 4, 3 );       // Vpp
    measurementLayout->setColumnStretch( 5, 3 );       // VDC
    measurementLayout->setColumnStretch( 6, 3 );       // Vac
    measurementLayout->setColumnStretch( 7, 4 );       // Vrms
    measurementLayout->setColumnStretch( 8, 3 );       // dB
    measurementLayout->setColumnStretch( 9, 3 );       // Power
    measurementLayout->setColumnStretch( 10, 2 );      // THD
    measurementLayout->setColumnStretch( 11, 3 );      // f
    measurementLayout->setColumnStretch( 12, 3 );      // note, cent
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        QPalette voltagePalette = palette;
        QPalette spectrumPalette = palette;
        voltagePalette.setColor( QPalette::WindowText, view->colors->voltage[ channel ] );
        spectrumPalette.setColor( QPalette::WindowText, view->colors->spectrum[ channel ] );
        measurementNameLabel.push_back( new QLabel( scope->voltage[ channel ].name ) );
        measurementNameLabel[ channel ]->setPalette( voltagePalette );
        measurementNameLabel[ channel ]->setAutoFillBackground( true );
        measurementMiscLabel.push_back( new QLabel() );
        measurementMiscLabel[ channel ]->setPalette( voltagePalette );
        measurementGainLabel.push_back( new QLabel() );
        measurementGainLabel[ channel ]->setPalette( voltagePalette );
        measurementMagnitudeLabel.push_back( new QLabel() );
        measurementMagnitudeLabel[ channel ]->setPalette( spectrumPalette );
        measurementVppLabel.push_back( new QLabel() );
        measurementVppLabel[ channel ]->setPalette( voltagePalette );
        measurementDCLabel.push_back( new QLabel() );
        measurementDCLabel[ channel ]->setPalette( voltagePalette );
        measurementACLabel.push_back( new QLabel() );
        measurementACLabel[ channel ]->setPalette( voltagePalette );
        measurementRMSLabel.push_back( new QLabel() );
        measurementRMSLabel[ channel ]->setPalette( voltagePalette );
        measurementdBLabel.push_back( new QLabel() );
        measurementdBLabel[ channel ]->setPalette( voltagePalette );
        measurementRMSPowerLabel.push_back( new QLabel() );
        measurementRMSPowerLabel[ channel ]->setPalette( voltagePalette );
        measurementTHDLabel.push_back( new QLabel() );
        measurementTHDLabel[ channel ]->setPalette( voltagePalette );
        measurementFrequencyLabel.push_back( new QLabel() );
        measurementFrequencyLabel[ channel ]->setPalette( voltagePalette );
        measurementNoteLabel.push_back( new QLabel() );
        measurementNoteLabel[ channel ]->setIndent( view->fontSize ); // provide about 1 char margin
        measurementNoteLabel[ channel ]->setPalette( voltagePalette );
        setMeasurementVisible( channel );
        int col = 0;
        measurementLayout->addWidget( measurementNameLabel[ channel ], int( channel ), col++, Qt::AlignLeft );
        measurementLayout->addWidget( measurementMiscLabel[ channel ], int( channel ), col++, Qt::AlignCenter );
        measurementLayout->addWidget( measurementGainLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementMagnitudeLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementVppLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementDCLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementACLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementRMSLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementdBLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementRMSPowerLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementTHDLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementFrequencyLabel[ channel ], int( channel ), col++, Qt::AlignRight );
        measurementLayout->addWidget( measurementNoteLabel[ channel ], int( channel ), col++, Qt::AlignLeft );
        if ( channel < spec->channels )
            updateVoltageCoupling( channel );
        else
            updateMathMode();
        updateVoltageDetails( channel );
        updateSpectrumDetails( channel );
    }

    // Cursors
    cursorDataGrid = new DataGrid( this );
    cursorDataGrid->setBackgroundColor( view->colors->background );
    cursorDataGrid->setToolTipsVisible( scope->toolTipVisible );

    cursorDataGrid->addItem( tr( "Markers" ), view->colors->text );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        cursorDataGrid->addItem( scope->voltage[ channel ].name, view->colors->voltage[ channel ] );
    }
    for ( ChannelID channel = 0; channel < scope->spectrum.size(); ++channel ) {
        cursorDataGrid->addItem( scope->spectrum[ channel ].name, view->colors->spectrum[ channel ] );
    }
    cursorDataGrid->selectItem( 0 );

    connect( cursorDataGrid, &DataGrid::itemSelected, mainScope, [ this ]( int index ) {
        mainScope->selectCursor( index );
        zoomScope->selectCursor( index );
        updateItem( ChannelID( index ), true );
    } );

    connect( cursorDataGrid, &DataGrid::itemUpdated, this, [ this ]( int index ) { updateItem( ChannelID( index ) ); } );

    scope->horizontal.cursor.shape = DsoSettingsScopeCursor::VERTICAL;

    // The layout for the widgets
    mainLayout = new QGridLayout();
    // Bars around the scope, needed because the slider-drawing-area is outside
    // the scope at min/max
    mainLayout->setColumnMinimumWidth( 2, mainSliders.triggerPositionSlider->preMargin() );
    mainLayout->setColumnMinimumWidth( 4, mainSliders.triggerPositionSlider->postMargin() );
    mainLayout->setSpacing( 0 );
    int row = 0;
    // display settings on top of scope, full width also with cursor grid (7 columns)
    mainLayout->addLayout( settingsLayout, row, 0, 1, 7 );
    ++row; // = 1
    // 5x5 box for mainScope & mainSliders & markerSlider
    mainLayout->addWidget( mainSliders.triggerPositionSlider, row, 2, 2, 3, Qt::AlignBottom );
    ++row; // = 2
    mainLayout->setRowMinimumHeight( row, mainSliders.voltageOffsetSlider->preMargin() );
    mainLayout->addWidget( mainSliders.voltageOffsetSlider, row, 1, 3, 2, Qt::AlignRight );
    mainLayout->addWidget( mainSliders.triggerLevelSlider, row, 4, 3, 2, Qt::AlignLeft );
    mainScopeRow = ++row; // = 3
    const int scopeCol = 3;
    mainLayout->setColumnStretch( scopeCol, 1 );  // Scopes increase their horizontal size
    mainLayout->setRowStretch( mainScopeRow, 1 ); // the scope gets max vertical space unless zoomed
    mainLayout->addWidget( mainScope, mainScopeRow, scopeCol );
    ++row; // = 4
    mainLayout->setRowMinimumHeight( row, mainSliders.voltageOffsetSlider->postMargin() );
    mainLayout->addWidget( mainSliders.markerSlider, row, 2, 2, 3, Qt::AlignTop );
    row += 2; // = 6, end 5x5 box
    // markerLayout
    mainLayout->addLayout( markerLayout, row, scopeCol, 1, 3 );
    ++row; // = 7
    // 5x4 box for zoomScope & zoomSliders
    mainLayout->addWidget( zoomSliders.triggerPositionSlider, row, 2, 2, 3, Qt::AlignBottom );
    ++row; // = 8
    mainLayout->setRowMinimumHeight( row, zoomSliders.voltageOffsetSlider->preMargin() );
    mainLayout->addWidget( zoomSliders.voltageOffsetSlider, row, 1, 3, 2, Qt::AlignRight );
    mainLayout->addWidget( zoomSliders.triggerLevelSlider, row, 4, 3, 2, Qt::AlignLeft );
    zoomScopeRow = ++row; // = 9
    // zoom scope consumes no space unless enabled in DW::updateZoom() below
    mainLayout->setRowStretch( zoomScopeRow, 0 );
    mainLayout->addWidget( zoomScope, zoomScopeRow, scopeCol );
    ++row; // = 10
    mainLayout->setRowMinimumHeight( row, zoomSliders.voltageOffsetSlider->postMargin() );
    // end 5x4 box
    // embedded measurementLayout
    const int measurementRow = ++row; // = 11
    // display channel measurements on bottom of scope, full width also with cursor grid (7 columns)
    mainLayout->addLayout( measurementLayout, measurementRow, 0, 1, -1 );
    updateCursorGrid( view->cursorsVisible );

    // The widget itself
    setPalette( palette );
    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );
    setLayout( mainLayout );

    // Connect change-signals of sliders
    connect( mainSliders.voltageOffsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset );
    connect( zoomSliders.voltageOffsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset );

    connect( mainSliders.triggerPositionSlider, &LevelSlider::valueChanged, this,
             [ this ]( int index, double value, bool pressed, QPoint globalPos ) {
                 updateTriggerPosition( index, value, pressed, globalPos, true );
             } );
    connect( zoomSliders.triggerPositionSlider, &LevelSlider::valueChanged, this,
             [ this ]( int index, double value, bool pressed, QPoint globalPos ) {
                 updateTriggerPosition( index, value, pressed, globalPos, false );
             } );

    connect( mainSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel );
    connect( zoomSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel );

    // show a horizontal level line as long as the trigger level slider is active
    connect( mainSliders.triggerLevelSlider, &LevelSlider::valueChanged, mainScope,
             [ this ]( int index, double value, bool pressed ) {
                 mainScope->generateGrid( index, value, pressed );
                 zoomScope->generateGrid( index, value, pressed );
             } );
    connect( zoomSliders.triggerLevelSlider, &LevelSlider::valueChanged, mainScope,
             [ this ]( int index, double value, bool pressed ) {
                 mainScope->generateGrid( index, value, pressed );
                 zoomScope->generateGrid( index, value, pressed );
             } );

    connect( mainSliders.markerSlider, &LevelSlider::valueChanged, [ this ]( int index, double value ) {
        updateMarker( unsigned( index ), value );
        mainScope->updateCursor();
        zoomScope->updateCursor();
    } );
    zoomSliders.markerSlider->setEnabled( false );
}


DsoWidget::~DsoWidget() {
    if ( scope->verboseLevel > 1 )
        qDebug() << " DsoWidget::~DsoWidget()";
}


void DsoWidget::switchToPrintColors() {
    if ( view->printerColorImages ) {
        view->colors = &view->print;
        setColors();
    }
}


void DsoWidget::restoreScreenColors() {
    if ( view->colors != &view->screen ) {
        view->colors = &view->screen;
        setColors();
    }
}


void DsoWidget::setColors() {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::setColors()";
    ChannelID numChannels = ChannelID( scope->voltage.size() );
    cursorDataGrid->setBackgroundColor( view->colors->background ); // switch cursor measurement
    cursorDataGrid->configureItem( 0, view->colors->text );         // and marker colors
    // Palette for this widget
    QPalette paletteNow;
    paletteNow.setColor( QPalette::Window, view->colors->background );
    paletteNow.setColor( QPalette::WindowText, view->colors->text );
    settingsSamplesOnScreen->setPalette( paletteNow );
    settingsSamplerateLabel->setPalette( paletteNow );
    settingsTimebaseLabel->setPalette( paletteNow );
    settingsFrequencybaseLabel->setPalette( paletteNow );
    markerInfoLabel->setPalette( paletteNow );
    markerTimeLabel->setPalette( paletteNow );
    markerFrequencyLabel->setPalette( paletteNow );
    markerTimebaseLabel->setPalette( paletteNow );
    markerFrequencybaseLabel->setPalette( paletteNow );
    QPalette tablePalette = paletteNow;
    for ( ChannelID channel = 0; channel < numChannels; ++channel ) {
        tablePalette.setColor( QPalette::WindowText, view->colors->spectrum[ channel ] );
        measurementMagnitudeLabel[ channel ]->setPalette( tablePalette );
        tablePalette.setColor( QPalette::WindowText, view->colors->voltage[ channel ] );
        measurementNameLabel[ channel ]->setPalette( tablePalette );
        measurementMiscLabel[ channel ]->setPalette( tablePalette );
        measurementGainLabel[ channel ]->setPalette( tablePalette );
        mainSliders.voltageOffsetSlider->setColor( ( channel ), view->colors->voltage[ channel ] );
        zoomSliders.voltageOffsetSlider->setColor( ( channel ), view->colors->voltage[ channel ] );
        mainSliders.voltageOffsetSlider->setColor( channel + numChannels, view->colors->spectrum[ channel ] );
        zoomSliders.voltageOffsetSlider->setColor( channel + numChannels, view->colors->spectrum[ channel ] );
        measurementVppLabel[ channel ]->setPalette( tablePalette );
        measurementDCLabel[ channel ]->setPalette( tablePalette );
        measurementACLabel[ channel ]->setPalette( tablePalette );
        measurementRMSLabel[ channel ]->setPalette( tablePalette );
        measurementdBLabel[ channel ]->setPalette( tablePalette );
        measurementRMSPowerLabel[ channel ]->setPalette( tablePalette );
        measurementTHDLabel[ channel ]->setPalette( tablePalette );
        measurementFrequencyLabel[ channel ]->setPalette( tablePalette );
        measurementNoteLabel[ channel ]->setPalette( tablePalette );
        cursorDataGrid->configureItem( channel + 1, view->colors->voltage[ channel ] ); // and voltage colors
        cursorDataGrid->configureItem( channel + numChannels + 1,
                                       view->colors->spectrum[ channel ] ); // and spectrum colors
    }

    tablePalette = palette();
    tablePalette.setColor( QPalette::WindowText, view->colors->voltage[ unsigned( scope->trigger.source ) ] );
    settingsTriggerLabel->setPalette( tablePalette );
    updateTriggerSource();
    setPalette( paletteNow );
    setBackgroundRole( QPalette::Window );
}


void DsoWidget::updateCursorGrid( bool enabled ) {
    if ( !enabled ) {
        cursorDataGrid->selectItem( 0 );
        cursorDataGrid->setParent( nullptr );
        mainScope->selectCursor( 0 );
        zoomScope->selectCursor( 0 );
        return;
    }
    // left or right of main and zoom, from mainScope down to (excluding) measurementLayout
    const int rows = mainLayout->rowCount() - mainScopeRow - 1;
    const int leftColumn = 0;
    const int rightColumn = mainLayout->columnCount() - 1;
    switch ( view->cursorGridPosition ) {
    case Qt::LeftToolBarArea:
        // keep space for settingsLayout on top and measurementLayout on bottom
        if ( mainLayout->itemAtPosition( mainScopeRow, leftColumn ) == nullptr ) {
            cursorDataGrid->setParent( nullptr );
            mainLayout->addWidget( cursorDataGrid, mainScopeRow, leftColumn, rows, 1 );
        }
        break;
    case Qt::RightToolBarArea:
        // keep space for settingsLayout on top and measurementLayout on bottom
        if ( mainLayout->itemAtPosition( mainScopeRow, rightColumn ) == nullptr ) {
            cursorDataGrid->setParent( nullptr );
            // right of main and zoom, from mainScope down to (excluding) measurementLayout
            mainLayout->addWidget( cursorDataGrid, mainScopeRow, rightColumn, rows, 1 );
        }
        break;
    default:
        if ( cursorDataGrid->parent() != nullptr ) {
            cursorDataGrid->setParent( nullptr );
        }
        break;
    }
}


void DsoWidget::updateItem( ChannelID index, bool switchOn ) {
    selectedCursor = index;
    ChannelID channelCount = scope->countChannels();
    if ( 0 < index && index < channelCount + 1 ) {
        ChannelID channel = ChannelID( index - 1 );
        if ( scope->voltage[ channel ].used ) {
            if ( switchOn || scope->voltage[ channel ].cursor.shape == DsoSettingsScopeCursor::NONE ) {
                scope->voltage[ channel ].cursor.shape = DsoSettingsScopeCursor::RECTANGULAR;
            } else {
                scope->voltage[ channel ].cursor.shape = DsoSettingsScopeCursor::NONE;
            }
        }
    } else if ( channelCount < index && index < 2 * channelCount + 1 ) {
        ChannelID channel = ChannelID( index - channelCount - 1 );
        if ( scope->spectrum[ channel ].used ) {
            if ( switchOn || scope->spectrum[ channel ].cursor.shape == DsoSettingsScopeCursor::NONE ) {
                scope->spectrum[ channel ].cursor.shape = DsoSettingsScopeCursor::RECTANGULAR;
            } else {
                scope->spectrum[ channel ].cursor.shape = DsoSettingsScopeCursor::NONE;
            }
        }
    }
    updateMarkerDetails();
    mainScope->updateCursor( int( index ) );
    zoomScope->updateCursor( int( index ) );
}


void DsoWidget::setupSliders( DsoWidget::Sliders &sliders ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::setupSliders()";
    // The offset sliders for all possible channels
    sliders.voltageOffsetSlider = new LevelSlider( Qt::RightArrow );
    if ( scope->toolTipVisible )
        sliders.voltageOffsetSlider->setToolTip( tr( "Trace position, drag the channel name up or down" ) );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        sliders.voltageOffsetSlider->addSlider( scope->voltage[ channel ].name, int( channel ) );
        sliders.voltageOffsetSlider->setColor( ( channel ), view->colors->voltage[ channel ] );
        sliders.voltageOffsetSlider->setLimits( int( channel ), -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2 );
        sliders.voltageOffsetSlider->setStep( int( channel ), 0.2 );
        sliders.voltageOffsetSlider->setValue( int( channel ), scope->voltage[ channel ].offset );
        sliders.voltageOffsetSlider->setIndexVisible( channel, scope->voltage[ channel ].used );
    }
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        sliders.voltageOffsetSlider->addSlider( scope->spectrum[ channel ].name, int( scope->voltage.size() + channel ) );
        sliders.voltageOffsetSlider->setColor( unsigned( scope->voltage.size() ) + channel, view->colors->spectrum[ channel ] );
        sliders.voltageOffsetSlider->setLimits( int( scope->voltage.size() + channel ), -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2 );
        sliders.voltageOffsetSlider->setStep( int( scope->voltage.size() + channel ), 0.2 );
        sliders.voltageOffsetSlider->setValue( int( scope->voltage.size() + channel ), scope->spectrum[ channel ].offset );
        sliders.voltageOffsetSlider->setIndexVisible( unsigned( scope->voltage.size() ) + channel,
                                                      scope->spectrum[ channel ].used );
    }

    // The triggerPosition slider
    sliders.triggerPositionSlider = new LevelSlider( Qt::DownArrow );
    if ( scope->toolTipVisible )
        sliders.triggerPositionSlider->setToolTip( tr( "Trigger position, drag the arrow left or right" ) );
    sliders.triggerPositionSlider->addSlider();
    sliders.triggerPositionSlider->setLimits( 0, 0.0, 1.0 );
    sliders.triggerPositionSlider->setStep( 0, 0.2 / double( DIVS_TIME ) );
    sliders.triggerPositionSlider->setValue( 0, scope->trigger.position );
    sliders.triggerPositionSlider->setIndexVisible( 0, true );

    // The sliders for the trigger levels
    sliders.triggerLevelSlider = new LevelSlider( Qt::LeftArrow );
    if ( scope->toolTipVisible )
        sliders.triggerLevelSlider->setToolTip( tr( "Trigger level, drag the arrow up or down" ) );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        sliders.triggerLevelSlider->addSlider( int( channel ) );
        sliders.triggerLevelSlider->setColor( channel, ( channel == ChannelID( scope->trigger.source ) )
                                                           ? view->colors->voltage[ channel ]
                                                           : view->colors->voltage[ channel ].darker() );
        adaptTriggerLevelSlider( sliders, channel );
        sliders.triggerLevelSlider->setValue( int( channel ), scope->voltage[ channel ].trigger );
        sliders.triggerLevelSlider->setIndexVisible( channel, scope->voltage[ channel ].used );
    }

    // The marker slider
    sliders.markerSlider = new LevelSlider( Qt::UpArrow );
    if ( scope->toolTipVisible )
        sliders.markerSlider->setToolTip( tr( "Measure or zoom marker '1' and '2', drag left or right" ) );
    for ( int marker = 0; marker < 2; ++marker ) {
        sliders.markerSlider->addSlider( QString::number( marker + 1 ), marker );
        sliders.markerSlider->setLimits( marker, MARGIN_LEFT, MARGIN_RIGHT );
        sliders.markerSlider->setStep( marker, MARKER_STEP );
        sliders.markerSlider->setValue( marker, scope->horizontal.cursor.pos[ marker ].x() );
        sliders.markerSlider->setIndexVisible( unsigned( marker ), true );
    }
}


/// \brief Set the trigger level sliders minimum and maximum to the new values.
void DsoWidget::adaptTriggerLevelSlider( DsoWidget::Sliders &sliders, ChannelID channel ) {
    // printf( "DW::adaptTriggerLevelSlider( %d )\n", channel );
    sliders.triggerLevelSlider->setLimits( int( channel ),
                                           ( -DIVS_VOLTAGE / 2 - scope->voltage[ channel ].offset ) * scope->gain( channel ),
                                           ( DIVS_VOLTAGE / 2 - scope->voltage[ channel ].offset ) * scope->gain( channel ) );
    sliders.triggerLevelSlider->setStep( int( channel ), scope->gain( channel ) * 0.05 );
    double value = sliders.triggerLevelSlider->value( int( channel ) );
    if ( bool( value ) ) { // ignore when first called at init
                           //        updateTriggerLevel(channel, value);
    }
}


/// \brief Show/Hide a line of the measurement table.
void DsoWidget::setMeasurementVisible( ChannelID channel ) {
    bool visible = scope->voltage[ channel ].used || scope->spectrum[ channel ].used;
    if ( visible ) { // enable this line
        measurementNameLabel[ channel ]->show();
        measurementMiscLabel[ channel ]->show();
        measurementGainLabel[ channel ]->show();
        measurementMagnitudeLabel[ channel ]->show();
        measurementVppLabel[ channel ]->show();
        measurementDCLabel[ channel ]->show();
        measurementACLabel[ channel ]->show();
        measurementRMSLabel[ channel ]->show();
        measurementdBLabel[ channel ]->show();
        measurementRMSPowerLabel[ channel ]->show();
        measurementTHDLabel[ channel ]->show();
        measurementFrequencyLabel[ channel ]->show();
        measurementNoteLabel[ channel ]->show();
        if ( scope->voltage[ channel ].used )
            measurementGainLabel[ channel ]->show();
        else
            measurementGainLabel[ channel ]->setText( QString() );
        if ( scope->spectrum[ channel ].used )
            measurementMagnitudeLabel[ channel ]->show();
        else
            measurementMagnitudeLabel[ channel ]->setText( QString() );
    } else { // do not show the line
        measurementNameLabel[ channel ]->hide();
        measurementMiscLabel[ channel ]->hide();
        measurementGainLabel[ channel ]->hide();
        measurementMagnitudeLabel[ channel ]->hide();
        measurementVppLabel[ channel ]->hide();
        measurementDCLabel[ channel ]->hide();
        measurementACLabel[ channel ]->hide();
        measurementRMSLabel[ channel ]->hide();
        measurementdBLabel[ channel ]->hide();
        measurementRMSPowerLabel[ channel ]->hide();
        measurementTHDLabel[ channel ]->hide();
        measurementFrequencyLabel[ channel ]->hide();
        measurementNoteLabel[ channel ]->hide();
    }
}


/// \brief Update the label about the marker measurements
void DsoWidget::updateMarkerDetails() {
    if ( nullptr == cursorDataGrid ) // not yet initialized
        return;
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateMarkerDetails()";
    double m1 = scope->horizontal.cursor.pos[ 0 ].x() + DIVS_TIME / 2; // zero at center -> zero at left margin
    double m2 = scope->horizontal.cursor.pos[ 1 ].x() + DIVS_TIME / 2; // zero at center -> zero at left margin
    if ( m1 > m2 )
        std::swap( m1, m2 );
    double divs = m2 - m1;
    // t = 0 at left screen margin
    double time0 = m1 * scope->horizontal.timebase;
    double time1 = m2 * scope->horizontal.timebase;
    double time = divs * scope->horizontal.timebase;
    double freq0 = m1 * scope->horizontal.frequencybase;
    double freq1 = m2 * scope->horizontal.frequencybase;
    double freq = divs * scope->horizontal.frequencybase;
    bool timeUsed = false;
    bool freqUsed = false;

    int index = 1;
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        if ( scope->voltage[ channel ].used ) {
            timeUsed = true; // at least one voltage channel used -> show marker time details
            QPointF p0 = scope->voltage[ channel ].cursor.pos[ 0 ];
            QPointF p1 = scope->voltage[ channel ].cursor.pos[ 1 ];
            if ( scope->voltage[ channel ].cursor.shape != DsoSettingsScopeCursor::NONE ) {
                cursorDataGrid->updateInfo(
                    unsigned( index ), true, tr( "ON" ),
                    valueToString( fabs( p1.x() - p0.x() ) * scope->horizontal.timebase, UNIT_SECONDS, 4 ),
                    valueToString( fabs( p1.y() - p0.y() ) * scope->gain( channel ), voltageUnits[ channel ], 4 ) );
            } else {
                cursorDataGrid->updateInfo( unsigned( index ), true, tr( "OFF" ), "", "" );
            }
        } else {
            cursorDataGrid->updateInfo( unsigned( index ), false );
        }
        ++index;
    }
    for ( ChannelID channel = 0; channel < scope->spectrum.size(); ++channel ) {
        if ( scope->spectrum[ channel ].used ) {
            freqUsed = true; // at least one spec channel used -> show marker freq details
            QPointF p0 = scope->spectrum[ channel ].cursor.pos[ 0 ];
            QPointF p1 = scope->spectrum[ channel ].cursor.pos[ 1 ];
            if ( scope->spectrum[ channel ].cursor.shape != DsoSettingsScopeCursor::NONE ) {
                cursorDataGrid->updateInfo(
                    unsigned( index ), true, tr( "ON" ),
                    valueToString( fabs( p1.x() - p0.x() ) * scope->horizontal.frequencybase, UNIT_HERTZ, 4 ),
                    valueToString( fabs( p1.y() - p0.y() ) * scope->spectrum[ channel ].magnitude, UNIT_DECIBEL, 4 ) +
                        scope->analysis.dBsuffix() );
            } else {
                cursorDataGrid->updateInfo( unsigned( index ), true, tr( "OFF" ), "", "" );
            }
        } else {
            cursorDataGrid->updateInfo( unsigned( index ), false );
        }
        ++index;
    }

    if ( divs >= DIVS_TIME || ( m1 <= 0 && m2 <= 0 ) || ( m1 >= DIVS_TIME && m2 >= DIVS_TIME ) ) {
        // markers at left/right margins -> don't display
        markerInfoLabel->setVisible( false );
        markerTimeLabel->setVisible( false );
        markerFrequencyLabel->setVisible( false );
        markerTimebaseLabel->setVisible( false );
        markerFrequencybaseLabel->setVisible( false );
    } else {
        markerInfoLabel->setVisible( true );
        markerTimeLabel->setVisible( true );
        markerFrequencyLabel->setVisible( true );
        markerTimebaseLabel->setVisible( view->zoom );
        markerFrequencybaseLabel->setVisible( view->zoom );
        QString mInfo( tr( "Markers  " ) );
        QString mTime( " t: " );
        QString mFreq( " f: " );
        if ( view->zoom ) {
            if ( divs != 0.0 ) {
                zoomFactor = DIVS_TIME / divs;
                mInfo = tr( "Zoom x%1  " ).arg( zoomFactor, -1, 'g', 3 );
            } else { // avoid div by zero
                zoomFactor = 1000;
                mInfo = tr( "Zoom ---  " );
            }
            markerTimebaseLabel->setText( "  " + valueToString( time / DIVS_TIME, UNIT_SECONDS, 3 ) + tr( "/div" ) );
            markerTimebaseLabel->setVisible( timeUsed );
            markerFrequencybaseLabel->setText( "  " + valueToString( freq / DIVS_TIME, UNIT_HERTZ, 3 ) + tr( "/div" ) );
            markerFrequencybaseLabel->setVisible( freqUsed );
        }
        markerInfoLabel->setText( mInfo );
        if ( timeUsed ) {
            mTime += QString( "%1" ).arg( valueToString( time0, UNIT_SECONDS, 3 ) );
            if ( time > 0 )
                mTime += QString( " ... %1,  Δt: %2 (%3) " )
                             .arg( valueToString( time1, UNIT_SECONDS, 3 ), valueToString( time, UNIT_SECONDS, 3 ),
                                   valueToString( 1 / time, UNIT_HERTZ, 3 ) );
            markerTimeLabel->setText( mTime );
        } else {
            markerTimeLabel->setText( "" );
        }
        if ( freqUsed ) {
            mFreq += QString( "%1" ).arg( valueToString( freq0, UNIT_HERTZ, 3 ) );
            if ( freq > 0 )
                mFreq += QString( " ... %2,  Δf: %3 " )
                             .arg( valueToString( freq1, UNIT_HERTZ, 3 ), valueToString( freq, UNIT_HERTZ, 3 ) );
            markerFrequencyLabel->setText( mFreq );
        } else
            markerFrequencyLabel->setText( "" );
    }

    QString markerMeasureTimeLabel;
    QString markerMeasureFreqLabel;
    if ( timeUsed ) {
        markerMeasureTimeLabel = QString( "Δt: %1" ).arg( valueToString( time, UNIT_SECONDS, 3 ) );
    }
    if ( freqUsed ) {
        markerMeasureFreqLabel = QString( "Δf: %1" ).arg( valueToString( freq, UNIT_HERTZ, 3 ) );
    }
    cursorDataGrid->updateInfo( unsigned( 0 ), true, QString(), markerMeasureTimeLabel, markerMeasureFreqLabel );
}


/// \brief Update the label about the spectrum settings
void DsoWidget::updateSpectrumDetails( ChannelID channel ) {
    setMeasurementVisible( channel );
    if ( scope->spectrum[ channel ].used )
        measurementMagnitudeLabel[ channel ]->setText( valueToString( scope->spectrum[ channel ].magnitude, UNIT_DECIBEL, 3 ) +
                                                       tr( "/div" ) );
    else
        measurementMagnitudeLabel[ channel ]->setText( QString() );
}


/// \brief Update the label about the trigger settings
void DsoWidget::updateTriggerDetails() {
    // Update the trigger details
    QPalette tablePalette = palette();
    tablePalette.setColor( QPalette::WindowText, view->colors->voltage[ unsigned( scope->trigger.source ) ] );
    settingsTriggerLabel->setPalette( tablePalette );
    QString levelString = valueToString( scope->voltage[ unsigned( scope->trigger.source ) ].trigger,
                                         voltageUnits[ size_t( scope->trigger.source ) ], 3 );
    QString pretriggerString = valueToString( scope->trigger.position * scope->horizontal.timebase * DIVS_TIME, UNIT_SECONDS, 3 );
    QString pre = Dso::slopeString( scope->trigger.slope ); // trigger slope
    QString post = pre;                                     // opposite trigger slope
    if ( scope->trigger.slope == Dso::Slope::Positive )
        post = Dso::slopeString( Dso::Slope::Negative );
    else if ( scope->trigger.slope == Dso::Slope::Negative )
        post = Dso::slopeString( Dso::Slope::Positive );
    QString pulseWidthString = bool( pulseWidth1 ) ? pre + valueToString( pulseWidth1, UNIT_SECONDS, 3 ) + post : "";
    pulseWidthString += bool( pulseWidth2 ) ? valueToString( pulseWidth2, UNIT_SECONDS, 3 ) + pre : "";
    if ( bool( pulseWidth1 ) && bool( pulseWidth2 ) ) {
        int dutyCyle = int( 0.5 + ( 100.0 * pulseWidth1 ) / ( pulseWidth1 + pulseWidth2 ) );
        pulseWidthString += " (" + QString::number( dutyCyle ) + "%)";
    }
    if ( !scope->liveCalibrationActive && scope->trigger.mode != Dso::TriggerMode::ROLL ) {
        settingsTriggerLabel->setText( tr( "%1  %2  %3  %4  %5" )
                                           .arg( scope->voltage[ unsigned( scope->trigger.source ) ].name,
                                                 Dso::slopeString( scope->trigger.slope ), levelString, pretriggerString,
                                                 pulseWidthString ) );
    } else {
        settingsTriggerLabel->setText( "" );
    }
}


/// \brief Update the label about the voltage settings
void DsoWidget::updateVoltageDetails( ChannelID channel ) {
    if ( channel >= scope->voltage.size() )
        return;

    setMeasurementVisible( channel );

    if ( scope->voltage[ channel ].used )
        measurementGainLabel[ channel ]->setText( valueToString( scope->gain( channel ), voltageUnits[ channel ], 3 ) +
                                                  tr( "/div" ) );
    else
        measurementGainLabel[ channel ]->setText( QString() );
}


/// \brief Handles frequencybaseChanged signal from the horizontal dock.
/// \param frequencybase The frequencybase used for displaying the trace.
void DsoWidget::updateFrequencybase( double frequencybase ) {
    settingsFrequencybaseLabel->setText( valueToString( frequencybase, UNIT_HERTZ, -1 ) + tr( "/div" ) );
    updateMarkerDetails();
}


/// \brief Updates the samplerate field after changing the samplerate.
/// \param samplerate The samplerate set in the oscilloscope.
void DsoWidget::updateSamplerate( double newSamplerate ) {
    samplerate = newSamplerate;
    scope->horizontal.dotsOnScreen = int( ceil( samplerate * timebase * DIVS_TIME ) );
    // printf( "DsoWidget::updateSamplerate( %g ) -> %d\n", samplerate, scope->horizontal.dotsOnScreen );
    settingsSamplerateLabel->setText( valueToString( samplerate, UNIT_SAMPLES, -1 ) + tr( "/s" ) );
}


/// \brief Handles timebaseChanged signal from the horizontal dock.
/// \param timebase The timebase used for displaying the trace.
void DsoWidget::updateTimebase( double newTimebase ) {
    timebase = newTimebase;
    scope->horizontal.dotsOnScreen = int( ceil( samplerate * timebase * DIVS_TIME ) );
    // printf( "DsoWidget::updateTimebase( %g ) -> %d\n", timebase, scope->horizontal.dotsOnScreen );
    settingsTimebaseLabel->setText( valueToString( timebase, UNIT_SECONDS, -1 ) + tr( "/div" ) );
    updateMarkerDetails();
}


/// \brief Handles magnitudeChanged signal from the spectrum dock.
/// \param channel The channel whose magnitude was changed.
void DsoWidget::updateSpectrumMagnitude( ChannelID channel ) {
    updateSpectrumDetails( channel );
    updateMarkerDetails();
}


/// \brief Handles usedChanged signal from the spectrum dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateSpectrumUsed( ChannelID channel, bool used ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateSpectrumUsed()" << channel << used;
    if ( channel >= scope->voltage.size() )
        return;
    bool spectrumUsed = false;
    for ( size_t ch = 0; ch < scope->voltage.size(); ++ch )
        if ( scope->spectrum[ ch ].used )
            spectrumUsed = true;
    settingsFrequencybaseLabel->setVisible( spectrumUsed ); // hide text if no spectrum channel used
    mainSliders.voltageOffsetSlider->setIndexVisible( unsigned( scope->voltage.size() ) + channel, used );
    zoomSliders.voltageOffsetSlider->setIndexVisible( unsigned( scope->voltage.size() ) + channel, used );

    updateSpectrumDetails( channel );
    updateMarkerDetails();
    if ( !used && selectedCursor == channel + scope->countChannels() + 1 )
        switchToMarker(); // active spectrum cursor no longer valid
}


/// \brief Handles modeChanged signal from the trigger dock.
void DsoWidget::updateTriggerMode() {
    updateTriggerDetails();
    mainSliders.triggerPositionSlider->setVisible( scope->trigger.mode != Dso::TriggerMode::ROLL );
    zoomSliders.triggerPositionSlider->setVisible( zoomScope->isVisible() && scope->trigger.mode != Dso::TriggerMode::ROLL );
}


/// \brief Handles slopeChanged signal from the trigger dock.
void DsoWidget::updateTriggerSlope() { updateTriggerDetails(); }


/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
    // Change the colors of the trigger sliders
    mainSliders.triggerPositionSlider->setColor( 0, view->colors->voltage[ unsigned( scope->trigger.source ) ] );
    zoomSliders.triggerPositionSlider->setColor( 0, view->colors->voltage[ unsigned( scope->trigger.source ) ] );

    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        QColor color = ( channel == unsigned( scope->trigger.source ) ) ? view->colors->voltage[ channel ]
                                                                        : view->colors->voltage[ channel ].darker();
        mainSliders.triggerLevelSlider->setColor( channel, color );
        zoomSliders.triggerLevelSlider->setColor( channel, color );
    }

    updateTriggerDetails();
}


/// \brief Handles couplingChanged signal from the voltage dock.
/// \param channel The channel whose coupling was changed.
void DsoWidget::updateVoltageCoupling( ChannelID channel ) {
    if ( channel >= scope->voltage.size() )
        return;
    measurementMiscLabel[ channel ]->setText( Dso::couplingString( scope->coupling( channel, spec ) ) );
}


/// \brief Handles modeChanged signal from the voltage dock.
void DsoWidget::updateMathMode() {
    ChannelID mathChannel = spec->channels;
    measurementMiscLabel[ mathChannel ]->setText( Dso::mathModeString( Dso::getMathMode( scope->voltage[ mathChannel ] ) ) );
    voltageUnits[ mathChannel ] = Dso::mathModeUnit( Dso::getMathMode( scope->voltage[ mathChannel ] ) );
    updateMarkerDetails();
}


/// \brief Handles gainChanged signal from the voltage dock.
/// \param channel The channel whose gain was changed.
void DsoWidget::updateVoltageGain( ChannelID channel ) {
    if ( channel >= scope->voltage.size() )
        return;
    adaptTriggerLevelSlider( mainSliders, channel );
    adaptTriggerLevelSlider( zoomSliders, channel );
    updateVoltageDetails( channel );

    updateMarkerDetails();
}


/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateVoltageUsed( ChannelID channel, bool used ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateVoltageUsed()" << channel << used;
    if ( channel >= scope->voltage.size() )
        return;

    mainSliders.voltageOffsetSlider->setIndexVisible( channel, used );
    zoomSliders.voltageOffsetSlider->setIndexVisible( channel, used );

    mainSliders.triggerLevelSlider->setIndexVisible( channel, used );
    zoomSliders.triggerLevelSlider->setIndexVisible( channel, used );

    setMeasurementVisible( channel );
    updateVoltageDetails( channel );
    updateMarkerDetails();
    if ( !used && selectedCursor == channel + 1 )
        switchToMarker(); // active voltage cursor no longer valid
}


/// \brief Change the record length.
void DsoWidget::updateRecordLength( int size ) {
    settingsSamplesOnScreen->setText( valueToString( double( size ), UNIT_SAMPLES, -1 ) + tr( " on screen" ) );
}


void DsoWidget::switchToMarker() {
    cursorDataGrid->selectItem( 0 ); // select marker button
    mainScope->selectCursor( 0 );    // and announce it to main ..
    zoomScope->selectCursor( 0 );    // .. and zoom scope
}


/// \brief Show/hide the zoom view.
void DsoWidget::updateZoom( bool enabled ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateZoom()" << enabled;
    cursorMeasurementValid = false;
    showCursorMessage(); // remove dangling tool tip
    // zoomed scope height in regards to main scope height if enabled, otherwise no space used
    mainLayout->setRowStretch( zoomScopeRow, enabled ? int( pow( 2, view->zoomHeightIndex ) ) : 0 );
    zoomScope->setVisible( enabled );
    zoomSliders.voltageOffsetSlider->setVisible( enabled );
    zoomSliders.triggerPositionSlider->setVisible( enabled );
    zoomSliders.triggerLevelSlider->setVisible( enabled );
    // Show time-/frequencybase and zoom factor if the magnified scope is shown
    markerLayout->setStretch( 3, enabled ? 1 : 0 );
    markerTimebaseLabel->setVisible( enabled );
    markerLayout->setStretch( 4, enabled ? 1 : 0 );
    markerFrequencybaseLabel->setVisible( enabled );
    updateMarkerDetails();
    repaint();
}


// increase / decrease zoomed window when scrolling in the black scope border (outside the glscope)
void DsoWidget::wheelEvent( QWheelEvent *event ) {
    if ( view->zoom ) {
        if ( event->angleDelta().y() > 0 && view->zoomHeightIndex < 4 ) {
            ++view->zoomHeightIndex;
            updateZoom( true );
        } else if ( event->angleDelta().y() < 0 && view->zoomHeightIndex > 0 ) {
            --view->zoomHeightIndex;
            updateZoom( true );
        }
    }
    event->accept();
}


/// \brief Prints analyzed data.
void DsoWidget::showNew( std::shared_ptr< PPresult > analysedData ) {
    if ( scope->verboseLevel > 4 )
        qDebug() << "    DsoWidget::showNew()" << analysedData->tag;
    mainScope->showData( analysedData );
    zoomScope->showData( analysedData );

    QPalette triggerLabelPalette = palette();
    if ( scope->liveCalibrationActive ) {
        swTriggerStatus->setText( tr( "<b> OFFSET CALIBRATION </b>" ) );
        triggerLabelPalette.setColor( QPalette::WindowText, Qt::black );
        triggerLabelPalette.setColor( QPalette::Window, Qt::red );
        swTriggerStatus->setPalette( triggerLabelPalette );
        swTriggerStatus->setVisible( true );
    } else if ( scope->trigger.mode == Dso::TriggerMode::ROLL ) {
        swTriggerStatus->setVisible( false );
    } else {
        swTriggerStatus->setText( tr( "TR" ) );
        triggerLabelPalette.setColor( QPalette::WindowText, Qt::black );
        triggerLabelPalette.setColor( QPalette::Window, analysedData->softwareTriggerTriggered ? Qt::green : Qt::red );
        swTriggerStatus->setPalette( triggerLabelPalette );
        swTriggerStatus->setVisible( true );
    }
    const size_t CH1 = 0;
    // const size_t CH2 = 1;
    const size_t MATH = 2;
    updateRecordLength( scope->horizontal.dotsOnScreen );
    pulseWidth1 = analysedData.get()->data( CH1 )->pulseWidth1;
    pulseWidth2 = analysedData.get()->data( CH1 )->pulseWidth2;
    voltageUnits[ MATH ] = analysedData.get()->data( MATH )->voltageUnit;
    updateTriggerDetails();

    QString uStr;
    QString mStr;
    double uCursor = INT_MIN;
    double mCursor = INT_MIN;
    bool uVisible = false;
    bool mVisible = false;

    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        if ( ( scope->voltage[ channel ].used || scope->spectrum[ channel ].used ) && analysedData.get()->data( channel ) ) {
            const DataChannel *data = analysedData.get()->data( channel );
            voltageUnits[ channel ] = data->voltageUnit; // V² for math multiply functions
            Unit voltageUnit = voltageUnits[ channel ];
            if ( cursorMeasurementValid ) { // right mouse button pressed, measure at mouse position
                // qDebug() << "visible" << channel << scope->voltage[ channel ].visible << scope->spectrum[ channel ].visible;
                // voltage and spec magnitude at cursor position
                uCursor = ( cursorMeasurementPosition.y() - scope->voltage[ channel ].offset ) * scope->gain( channel );
                mCursor =
                    ( cursorMeasurementPosition.y() - scope->spectrum[ channel ].offset ) * scope->spectrum[ channel ].magnitude;
                // are u and m values inside or near (+- 20% of division) this visible trace?
                if ( scope->voltage[ channel ].visible ) {
                    uVisible = true;
                    if ( uCursor > data->vmin - 0.2 * scope->gain( channel ) &&
                         uCursor <= data->vmax + 0.2 * scope->gain( channel ) )
                        uStr += '\t' + scope->voltage[ channel ].name + ": " + valueToString( uCursor, voltageUnit, 3 );
                }
                if ( scope->spectrum[ channel ].visible ) {
                    mVisible = true;
                    if ( mCursor > data->dBmin - 0.2 * scope->spectrum[ channel ].magnitude &&
                         mCursor <= data->dBmax + 0.2 * scope->spectrum[ channel ].magnitude )
                        mStr += '\t' + scope->spectrum[ channel ].name + ": " + valueToString( mCursor, UNIT_DECIBEL, 3 ) +
                                scope->analysis.dBsuffix();
                }
            }
            // Vpp Amplitude string representation (3 significant digits)
            measurementVppLabel[ channel ]->setText( valueToString( data->vmax - data->vmin, voltageUnit, 3 ) + tr( "pp" ) );
            // DC Amplitude string representation (3 significant digits)
            measurementDCLabel[ channel ]->setText( valueToString( data->dc, voltageUnit, 3 ) + "=" );
            // AC Amplitude string representation (3 significant digits)
            measurementACLabel[ channel ]->setText( valueToString( data->ac, voltageUnit, 3 ) + "~" );
            // RMS Amplitude string representation (3 significant digits)
            measurementRMSLabel[ channel ]->setText( valueToString( data->rms, voltageUnit, 3 ) + tr( "rms" ) );
            // dB Amplitude string representation (3 significant digits)
            measurementdBLabel[ channel ]->setText( valueToString( data->dB, UNIT_DECIBEL, 3 ) + scope->analysis.dBsuffix() );
            // Frequency string representation (3 significant digits)
            measurementFrequencyLabel[ channel ]->setText( valueToString( data->frequency, UNIT_HERTZ, 4 ) );
            // Frequency note representation
            if ( scope->analysis.showNoteValue ) {
                measurementLayout->setColumnStretch( 12, 3 );
                measurementNoteLabel[ channel ]->setText( data->note );
            } else { // do not show this label
                measurementNoteLabel[ channel ]->setText( "" );
                measurementLayout->setColumnStretch( 12, 0 ); // Note
            }
            // RMS Amplitude string representation (3 significant digits)
            if ( scope->analysis.calculateDummyLoad && scope->analysis.dummyLoad > 0 ) {
                measurementLayout->setColumnStretch( 9, 3 );
                measurementRMSPowerLabel[ channel ]->setText(
                    valueToString( ( data->rms * data->rms ) / scope->analysis.dummyLoad, UNIT_WATTS, 3 ) );
            } else { // do not show this label
                measurementRMSPowerLabel[ channel ]->setText( "" );
                measurementLayout->setColumnStretch( 9, 0 ); // Power
            }
            if ( scope->analysis.calculateTHD ) {
                double thd = data->thd;
                measurementLayout->setColumnStretch( 10, 2 );
                if ( thd > 0 ) // display either xx.x% or xxx%
                    measurementTHDLabel[ channel ]->setText( QString( "%1%" ).arg( thd * 100, 4, 'f', thd < 1 ? 1 : 0 ) );
                else // invalid, blank label
                    measurementTHDLabel[ channel ]->setText( "" );
            } else { // do not show this label
                measurementTHDLabel[ channel ]->setText( "" );
                measurementLayout->setColumnStretch( 10, 0 ); // THD
            }
        }

        // Highlight clipped channel
        QPalette validPalette;
        if ( analysedData.get()->data( channel )->valid ) { // normal display
            validPalette.setColor( QPalette::WindowText, view->colors->voltage[ channel ] );
            validPalette.setColor( QPalette::Window, view->colors->background );
        } else { // warning
            validPalette.setColor( QPalette::WindowText, Qt::black );
            validPalette.setColor( QPalette::Window, Qt::red );
        }
        measurementNameLabel[ channel ]->setPalette( validPalette );
    }

    if ( cursorMeasurementValid ) {
        QString measurement;
        // show time if inside voltage trace or outside of all traces
        if ( uVisible && ( !uStr.isEmpty() || ( uStr.isEmpty() && mStr.isEmpty() ) ) ) {
            measurement +=
                valueToString( ( cursorMeasurementPosition.x() + DIVS_TIME / 2.0 ) * scope->horizontal.timebase, UNIT_SECONDS, 3 );
            measurement += '\t' + uStr;
        }
        // show frequency if inside spectrum trace or outside of all traces
        if ( mVisible && ( !mStr.isEmpty() || ( uStr.isEmpty() && mStr.isEmpty() ) ) ) {
            if ( !measurement.isEmpty() )
                measurement += '\n';
            measurement += valueToString( ( cursorMeasurementPosition.x() + DIVS_TIME / 2.0 ) * scope->horizontal.frequencybase,
                                          UNIT_HERTZ, 3 );
            measurement += '\t' + mStr;
        }
        if ( !measurement.isEmpty() ) {
            showCursorMessage( cursorGlobalPosition, measurement );
        }
    }
}


void DsoWidget::showCursorMessage( QPoint globalPos, const QString &message ) {
    if ( scope->verboseLevel > 3 )
        qDebug() << "   DsoWidget::showCursorMessage()" << globalPos << message;
    QToolTip::showText( globalPos, message );
}


void DsoWidget::showEvent( QShowEvent *event ) {
    QWidget::showEvent( event );
    // Apply settings and update measured values
    updateTriggerDetails();
    updateRecordLength( scope->horizontal.recordLength );
    updateFrequencybase( scope->horizontal.frequencybase );
    updateSamplerate( scope->horizontal.samplerate );
    updateTimebase( scope->horizontal.timebase );
    updateZoom( view->zoom );

    updateTriggerSource();
    adaptTriggerPositionSlider();
}


/// \brief Handles valueChanged signal from the offset sliders.
/// \param channel The channel whose offset was changed.
/// \param value The new offset for the channel.
void DsoWidget::updateOffset( ChannelID channel, double value, bool pressed, QPoint globalPos ) {
    if ( channel < scope->voltage.size() ) {
        scope->voltage[ channel ].offset = value;
        adaptTriggerLevelSlider( mainSliders, channel );
        adaptTriggerLevelSlider( zoomSliders, channel );
    } else if ( channel < scope->voltage.size() * 2 )
        scope->spectrum[ channel - scope->voltage.size() ].offset = value;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if ( channel < scope->voltage.size() * 2 ) {
        if ( mainSliders.voltageOffsetSlider->value( int( channel ) ) != value ) { // double != comparison is safe in this case
            const QSignalBlocker blocker( mainSliders.voltageOffsetSlider );
            mainSliders.voltageOffsetSlider->setValue( int( channel ), value );
        }
        if ( zoomSliders.voltageOffsetSlider->value( int( channel ) ) != value ) { // double != comparison is safe in this case
            const QSignalBlocker blocker( zoomSliders.voltageOffsetSlider );
            zoomSliders.voltageOffsetSlider->setValue( int( channel ), value );
        }
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateOffset()" << channel << value << pressed << globalPos;

    emit voltageOffsetChanged( channel, value );
}


/// \brief Translate horizontal position (0..1) from main view to zoom view.
double DsoWidget::mainToZoom( double position ) const {
    double m1 = scope->getMarker( 0 );
    double m2 = scope->getMarker( 1 );
    if ( m1 > m2 )
        std::swap( m1, m2 );
    return ( ( position - 0.5 ) * DIVS_TIME - m1 ) / std::max( m2 - m1, 1e-9 );
}


/// \brief Translate horizontal position (0..1) from zoom view to main view.
double DsoWidget::zoomToMain( double position ) const {
    double m1 = scope->getMarker( 0 );
    double m2 = scope->getMarker( 1 );
    if ( m1 > m2 )
        std::swap( m1, m2 );
    return 0.5 + ( m1 + position * ( m2 - m1 ) ) / DIVS_TIME;
}


/// \brief Handles signals affecting trigger position in the zoom view.
void DsoWidget::adaptTriggerPositionSlider() {
    double value = mainToZoom( scope->trigger.position );

    LevelSlider &slider = *zoomSliders.triggerPositionSlider;
    const QSignalBlocker blocker( slider );
    if ( slider.minimum( 0 ) <= value && value <= slider.maximum( 0 ) ) {
        slider.setEnabled( true );
        slider.setValue( 0, value );
    } else {
        slider.setEnabled( false );
        if ( value < slider.minimum( 0 ) ) {
            slider.setValue( 0, slider.minimum( 0 ) );
        } else {
            slider.setValue( 0, slider.maximum( 0 ) );
        }
    }
}


/// \brief Handles valueChanged signal from the triggerPosition slider.
/// \param index The index of the slider.
/// \param value The new triggerPosition in seconds relative to the first
/// sample.
void DsoWidget::updateTriggerPosition( int index, double value, bool pressed, QPoint globalPos, bool mainView ) {
    if ( index != 0 )
        return;

    if ( mainView ) {
        scope->trigger.position = value;
        adaptTriggerPositionSlider();
    } else {
        scope->trigger.position = zoomToMain( value );
        const QSignalBlocker blocker( mainSliders.triggerPositionSlider );
        mainSliders.triggerPositionSlider->setValue( index, scope->trigger.position );
    }

    updateTriggerDetails();
    updateMarkerDetails();
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateTriggerPosition()" << index << scope->trigger.position << pressed << globalPos;
    emit triggerPositionChanged( scope->trigger.position );
    if ( pressed ) {
        int resolution = 3;
        if ( !mainView )
            resolution += int( log10( zoomFactor ) );
        showCursorMessage( globalPos, valueToString( scope->trigger.position * scope->horizontal.timebase * DIVS_TIME, UNIT_SECONDS,
                                                     resolution ) );
    } else
        showCursorMessage();
}


/// \brief Handles valueChanged signal from the trigger level slider.
/// \param channel The index of the slider.
/// \param value The new trigger level.
void DsoWidget::updateTriggerLevel( ChannelID channel, double value, bool pressed, QPoint globalPos ) {
    scope->voltage[ channel ].trigger = value;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if ( mainSliders.triggerLevelSlider->value( int( channel ) ) != value ) { // double != comparison is safe in this case
        const QSignalBlocker blocker( mainSliders.triggerLevelSlider );
        mainSliders.triggerLevelSlider->setValue( int( channel ), value );
    }
    if ( zoomSliders.triggerLevelSlider->value( int( channel ) ) != value ) { // double != comparison is safe in this case
        const QSignalBlocker blocker( zoomSliders.triggerLevelSlider );
        zoomSliders.triggerLevelSlider->setValue( int( channel ), value );
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    updateTriggerDetails();
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateTriggerValue()" << channel << value << pressed << globalPos;
    emit triggerLevelChanged( channel, value );
    if ( pressed )
        showCursorMessage( globalPos, valueToString( value, voltageUnits[ channel ], 3 ) );
    else
        showCursorMessage();
}


/// \brief Handles valueChanged signal from the marker slider.
/// \param marker The index of the slider.
/// \param value The new marker position.
void DsoWidget::updateMarker( unsigned marker, double value ) {
    if ( scope->verboseLevel > 3 )
        qDebug() << "   DsoWidget::updateMarker()" << marker << value;
    scope->setMarker( marker, value );
    adaptTriggerPositionSlider();
    updateMarkerDetails();
}


/// \brief Update the sliders settings.
void DsoWidget::updateSlidersSettings() {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  DsoWidget::updateSlidersSettings()";
    // The offset sliders for all possible channels
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        updateOffset( channel, scope->voltage[ channel ].offset, false, QPoint() );
        mainSliders.voltageOffsetSlider->setColor( ( channel ), view->colors->voltage[ channel ] );
        mainSliders.voltageOffsetSlider->setValue( int( channel ), scope->voltage[ channel ].offset );
        mainSliders.voltageOffsetSlider->setIndexVisible( channel, scope->voltage[ channel ].used );
        zoomSliders.voltageOffsetSlider->setColor( ( channel ), view->colors->voltage[ channel ] );
        zoomSliders.voltageOffsetSlider->setValue( int( channel ), scope->voltage[ channel ].offset );
        zoomSliders.voltageOffsetSlider->setIndexVisible( channel, scope->voltage[ channel ].used );

        updateOffset( unsigned( scope->voltage.size() + channel ), scope->spectrum[ channel ].offset, false, QPoint() );
        mainSliders.voltageOffsetSlider->setColor( unsigned( scope->voltage.size() ) + channel, view->colors->spectrum[ channel ] );
        mainSliders.voltageOffsetSlider->setValue( int( scope->voltage.size() + channel ), scope->spectrum[ channel ].offset );
        mainSliders.voltageOffsetSlider->setIndexVisible( unsigned( scope->voltage.size() ) + channel,
                                                          scope->spectrum[ channel ].used );
        zoomSliders.voltageOffsetSlider->setColor( unsigned( scope->voltage.size() ) + channel, view->colors->spectrum[ channel ] );
        zoomSliders.voltageOffsetSlider->setValue( int( scope->voltage.size() + channel ), scope->spectrum[ channel ].offset );
        zoomSliders.voltageOffsetSlider->setIndexVisible( unsigned( scope->voltage.size() ) + channel,
                                                          scope->spectrum[ channel ].used );
    }

    // The trigger position slider
    mainSliders.triggerPositionSlider->setValue( 0, scope->trigger.position );
    updateTriggerPosition( 0, scope->trigger.position, false, QPoint(), true );                // main slider
    updateTriggerPosition( 0, mainToZoom( scope->trigger.position ), false, QPoint(), false ); // zoom slider

    // The sliders for the trigger levels
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        mainSliders.triggerLevelSlider->setValue( int( channel ), scope->voltage[ channel ].trigger );
        adaptTriggerLevelSlider( mainSliders, channel );
        mainSliders.triggerLevelSlider->setColor( channel, ( channel == unsigned( scope->trigger.source ) )
                                                               ? view->colors->voltage[ channel ]
                                                               : view->colors->voltage[ channel ].darker() );
        mainSliders.triggerLevelSlider->setIndexVisible( channel, scope->voltage[ channel ].used );
        zoomSliders.triggerLevelSlider->setValue( int( channel ), scope->voltage[ channel ].trigger );
        adaptTriggerLevelSlider( zoomSliders, channel );
        zoomSliders.triggerLevelSlider->setColor( channel, ( channel == unsigned( scope->trigger.source ) )
                                                               ? view->colors->voltage[ channel ]
                                                               : view->colors->voltage[ channel ].darker() );
        zoomSliders.triggerLevelSlider->setIndexVisible( channel, scope->voltage[ channel ].used );
    }
    updateTriggerDetails();

    // The marker slider
    for ( int marker = 0; marker < 2; ++marker ) {
        mainSliders.markerSlider->setValue( marker, scope->horizontal.cursor.pos[ marker ].x() );
    }
    updateMarkerDetails();
}
