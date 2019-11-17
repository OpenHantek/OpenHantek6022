// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QTimer>

#include "dsowidget.h"

#include "post/postprocessingsettings.h"
#include "post/graphgenerator.h"
#include "post/ppresult.h"

#include "utils/printutils.h"

#include "glscope.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "viewsettings.h"
#include "widgets/levelslider.h"
#include "widgets/datagrid.h"

static int zoomScopeRow = 0;

DsoWidget::DsoWidget(DsoSettingsScope *scope, DsoSettingsView *view, const Dso::ControlSpecification *spec,
                     QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags), scope(scope), view(view), spec(spec), mainScope(GlScope::createNormal(scope, view)),
      zoomScope(GlScope::createZoomed(scope, view)) {

    // Palette for this widget
    QPalette palette;
    palette.setColor(QPalette::Background, view->screen.background);
    palette.setColor(QPalette::WindowText, view->screen.text);

    setupSliders(mainSliders);
    setupSliders(zoomSliders);

    connect(mainScope, &GlScope::markerMoved, [this](unsigned cursorIndex, unsigned marker) {
        mainSliders.markerSlider->setValue(marker, this->scope->getMarker(marker));
        mainScope->updateCursor(cursorIndex);
        zoomScope->updateCursor(cursorIndex);
    });
    connect(zoomScope, &GlScope::markerMoved, [this](unsigned cursorIndex) {
        mainScope->updateCursor(cursorIndex);
        zoomScope->updateCursor(cursorIndex);
    });

    // The table for the settings at screen top
    settingsTriggerLabel = new QLabel();
    settingsTriggerLabel->setMinimumWidth(320);
    settingsTriggerLabel->setIndent(5);
    settingsSamplesOnScreen = new QLabel();
    settingsSamplesOnScreen->setAlignment(Qt::AlignRight);
    settingsSamplesOnScreen->setPalette(palette);
    settingsSamplerateLabel = new QLabel();
    settingsSamplerateLabel->setAlignment(Qt::AlignRight);
    settingsSamplerateLabel->setPalette(palette);
    settingsTimebaseLabel = new QLabel();
    settingsTimebaseLabel->setAlignment(Qt::AlignRight);
    settingsTimebaseLabel->setPalette(palette);
    settingsFrequencybaseLabel = new QLabel();
    settingsFrequencybaseLabel->setAlignment(Qt::AlignRight);
    settingsFrequencybaseLabel->setPalette(palette);
    swTriggerStatus = new QLabel();
    swTriggerStatus->setMinimumWidth(20);
    swTriggerStatus->setText(tr("TR"));
    swTriggerStatus->setAlignment(Qt::AlignCenter);
    swTriggerStatus->setAutoFillBackground(true);
    swTriggerStatus->setVisible(false);
    settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget(swTriggerStatus);
    settingsLayout->addWidget(settingsTriggerLabel);
    settingsLayout->addWidget(settingsSamplesOnScreen, 1);
    settingsLayout->addWidget(settingsSamplerateLabel, 1);
    settingsLayout->addWidget(settingsTimebaseLabel, 1);
    settingsLayout->addWidget(settingsFrequencybaseLabel, 1);

    // The table for the marker details
    markerInfoLabel = new QLabel();
    markerInfoLabel->setAlignment(Qt::AlignLeft);
    markerInfoLabel->setPalette(palette);
    markerTimeLabel = new QLabel();
    markerTimeLabel->setAlignment(Qt::AlignLeft);
    markerTimeLabel->setPalette(palette);
    markerFrequencyLabel = new QLabel();
    markerFrequencyLabel->setAlignment(Qt::AlignLeft);
    markerFrequencyLabel->setPalette(palette);
    markerTimebaseLabel = new QLabel();
    markerTimebaseLabel->setAlignment(Qt::AlignRight);
    markerTimebaseLabel->setPalette(palette);
    markerFrequencybaseLabel = new QLabel();
    markerFrequencybaseLabel->setAlignment(Qt::AlignRight);
    markerFrequencybaseLabel->setPalette(palette);
    markerLayout = new QHBoxLayout();
    markerLayout->addWidget(markerInfoLabel);
    markerLayout->addWidget(markerTimeLabel, 1);
    markerLayout->addWidget(markerFrequencyLabel, 1);
    markerLayout->addWidget(markerTimebaseLabel, 1);
    markerLayout->addWidget(markerFrequencybaseLabel, 1);

    // The table for the measurements at screen bottom
    QPalette tablePalette = palette;
    measurementLayout = new QGridLayout();
    int iii=0;
    measurementLayout->setColumnMinimumWidth(iii++, 50);
    measurementLayout->setColumnMinimumWidth(iii++, 30);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    measurementLayout->setColumnStretch(iii++, 3);
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        tablePalette.setColor(QPalette::WindowText, view->screen.voltage[channel]);
        measurementNameLabel.push_back(new QLabel(scope->voltage[channel].name));
        measurementNameLabel[channel]->setAlignment(Qt::AlignCenter);
        measurementNameLabel[channel]->setPalette(tablePalette);
        measurementNameLabel[channel]->setAutoFillBackground(true);
        measurementMiscLabel.push_back(new QLabel());
        measurementMiscLabel[channel]->setAlignment(Qt::AlignLeft);
        measurementMiscLabel[channel]->setPalette(tablePalette);
        measurementGainLabel.push_back(new QLabel());
        measurementGainLabel[channel]->setAlignment(Qt::AlignRight);
        measurementGainLabel[channel]->setPalette(tablePalette);
        tablePalette.setColor(QPalette::WindowText, view->screen.spectrum[channel]);
        measurementMagnitudeLabel.push_back(new QLabel());
        measurementMagnitudeLabel[channel]->setAlignment(Qt::AlignRight);
        measurementMagnitudeLabel[channel]->setPalette(tablePalette);
        measurementVppLabel.push_back(new QLabel());
        measurementVppLabel[channel]->setAlignment(Qt::AlignRight);
        measurementVppLabel[channel]->setPalette(palette);
        measurementRMSLabel.push_back(new QLabel());
        measurementRMSLabel[channel]->setAlignment(Qt::AlignRight);
        measurementRMSLabel[channel]->setPalette(palette);
        measurementDCLabel.push_back(new QLabel());
        measurementDCLabel[channel]->setAlignment(Qt::AlignRight);
        measurementDCLabel[channel]->setPalette(palette);
        measurementACLabel.push_back(new QLabel());
        measurementACLabel[channel]->setAlignment(Qt::AlignRight);
        measurementACLabel[channel]->setPalette(palette);
        measurementdBLabel.push_back(new QLabel());
        measurementdBLabel[channel]->setAlignment(Qt::AlignRight);
        measurementdBLabel[channel]->setPalette(palette);
        measurementFrequencyLabel.push_back(new QLabel());
        measurementFrequencyLabel[channel]->setAlignment(Qt::AlignRight);
        measurementFrequencyLabel[channel]->setPalette(palette);
        setMeasurementVisible(channel);
        iii = 0;
        measurementLayout->addWidget(measurementNameLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementMiscLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementGainLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementMagnitudeLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementVppLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementRMSLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementDCLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementACLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementdBLabel[channel], (int)channel, iii++);
        measurementLayout->addWidget(measurementFrequencyLabel[channel], (int)channel, iii++);
        if ((unsigned)channel < spec->channels)
            updateVoltageCoupling((unsigned)channel);
        else
            updateMathMode();
        updateVoltageDetails((unsigned)channel);
        updateSpectrumDetails((unsigned)channel);
    }

    // Cursors
    cursorDataGrid = new DataGrid(this);
    cursorDataGrid->setBackgroundColor(view->screen.background);
    cursorDataGrid->addItem(tr("Markers"), view->screen.text);
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        cursorDataGrid->addItem(scope->voltage[channel].name, view->screen.voltage[channel]);
    }
    for (ChannelID channel = 0; channel < scope->spectrum.size(); ++channel) {
        cursorDataGrid->addItem(scope->spectrum[channel].name, view->screen.spectrum[channel]);
    }
    cursorDataGrid->selectItem(0);

    connect(cursorDataGrid, &DataGrid::itemSelected, [this] (unsigned index) {
        mainScope->cursorSelected(index);
        zoomScope->cursorSelected(index);
    });
    connect(cursorDataGrid, &DataGrid::itemUpdated, [this, scope] (unsigned index) {
        unsigned channelCount = scope->countChannels();
        if (0 < index && index < channelCount + 1) {
            ChannelID channel = index - 1;
            if (scope->voltage[channel].used) {
                unsigned shape = (unsigned)scope->voltage[channel].cursor.shape;
                if (shape == DsoSettingsScopeCursor::NONE) {
                    scope->voltage[channel].cursor.shape = DsoSettingsScopeCursor::RECTANGULAR;
                } else {
                    scope->voltage[channel].cursor.shape = DsoSettingsScopeCursor::NONE;
                }
            }
        } else if (channelCount < index && index < 2 * channelCount + 1) {
            ChannelID channel = index - channelCount - 1;
            if (scope->spectrum[channel].used) {
                unsigned shape = (unsigned)scope->spectrum[channel].cursor.shape;
                if (shape == DsoSettingsScopeCursor::NONE) {
                    scope->spectrum[channel].cursor.shape = DsoSettingsScopeCursor::RECTANGULAR;
                } else {
                    scope->spectrum[channel].cursor.shape = DsoSettingsScopeCursor::NONE;
                }
            }
        }
        updateMarkerDetails();
        mainScope->updateCursor(index);
        zoomScope->updateCursor(index);
    });

    scope->horizontal.cursor.shape = DsoSettingsScopeCursor::VERTICAL;

    // The layout for the widgets
    mainLayout = new QGridLayout();
    mainLayout->setColumnStretch(3, 1); // Scopes increase their size
    // Bars around the scope, needed because the slider-drawing-area is outside
    // the scope at min/max
    mainLayout->setColumnMinimumWidth(2, mainSliders.triggerPositionSlider->preMargin());
    mainLayout->setColumnMinimumWidth(4, mainSliders.triggerPositionSlider->postMargin());
    mainLayout->setSpacing(0);
    int row = 0;
    mainLayout->addLayout(settingsLayout, row++, 1, 1, 5);
    // 5x5 box for mainScope & mainSliders
    mainLayout->setRowMinimumHeight(row + 1, mainSliders.offsetSlider->preMargin());
    mainLayout->setRowMinimumHeight(row + 3, mainSliders.offsetSlider->postMargin());
    mainLayout->setRowStretch(row + 2, 1);
    mainLayout->addWidget(mainScope, row + 2, 3);
    mainLayout->addWidget(mainSliders.offsetSlider, row + 1, 1, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(mainSliders.triggerPositionSlider, row, 2, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(mainSliders.triggerLevelSlider, row + 1, 4, 3, 2, Qt::AlignLeft);
    mainLayout->addWidget(mainSliders.markerSlider, row + 3, 2, 2, 3, Qt::AlignTop);
    row += 5;
    // Separators and markerLayout
    mainLayout->setRowMinimumHeight(row++, 5);
    mainLayout->addLayout(markerLayout, row++, 1, 1, 5);
    mainLayout->setRowMinimumHeight(row++, 4);
    // 5x5 box for zoomScope & zoomSliders
    zoomScopeRow = row + 2;
    mainLayout->addWidget(zoomScope, zoomScopeRow, 3);
    mainLayout->addWidget(zoomSliders.offsetSlider, row + 1, 1, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(zoomSliders.triggerPositionSlider, row, 2, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(zoomSliders.triggerLevelSlider, row + 1, 4, 3, 2, Qt::AlignLeft);
    row += 5;
    // Separator and embedded measurementLayout
    mainLayout->setRowMinimumHeight(row++, 8);
    mainLayout->addLayout(measurementLayout, row++, 1, 1, 5);

    updateCursorGrid(view->cursorsVisible);

    // The widget itself
    setPalette(palette);
    setBackgroundRole(QPalette::Background);
    setAutoFillBackground(true);
    setLayout(mainLayout);

    // Connect change-signals of sliders
    connect(mainSliders.offsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset);
    connect(zoomSliders.offsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset);

    connect(mainSliders.triggerPositionSlider, &LevelSlider::valueChanged, [this](int index, double value) {
        updateTriggerPosition(index, value, true);
    });
    connect(zoomSliders.triggerPositionSlider, &LevelSlider::valueChanged, [this](int index, double value) {
        updateTriggerPosition(index, value, false);
    });

    connect(mainSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel);
    connect(zoomSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel);

    connect(mainSliders.markerSlider, &LevelSlider::valueChanged, [this](int index, double value) {
        updateMarker(index, value);
        mainScope->updateCursor();
        zoomScope->updateCursor();
    });
    zoomSliders.markerSlider->setEnabled(false);
}

void DsoWidget::updateCursorGrid(bool enabled) {
    if (!enabled) {
        cursorDataGrid->selectItem(0);
        cursorDataGrid->setParent(nullptr);
        mainScope->cursorSelected(0);
        zoomScope->cursorSelected(0);
        return;
    }

    switch (view->cursorGridPosition) {
    case Qt::LeftToolBarArea:
        if (mainLayout->itemAtPosition(0, 0) == nullptr) {
            cursorDataGrid->setParent(nullptr);
            mainLayout->addWidget(cursorDataGrid, 0, 0, mainLayout->rowCount(), 1);
        }
        break;
    case Qt::RightToolBarArea:
        if (mainLayout->itemAtPosition(0, 6) == nullptr) {
            cursorDataGrid->setParent(nullptr);
            mainLayout->addWidget(cursorDataGrid, 0, 6, mainLayout->rowCount(), 1);
        }
        break;
    default:
        if (cursorDataGrid->parent() != nullptr) {
            cursorDataGrid->setParent(nullptr);
        }
        break;
    }
}

void DsoWidget::setupSliders(DsoWidget::Sliders &sliders) {
    // The offset sliders for all possible channels
    sliders.offsetSlider = new LevelSlider(Qt::RightArrow);
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        sliders.offsetSlider->addSlider(scope->voltage[channel].name, channel);
        sliders.offsetSlider->setColor(channel, view->screen.voltage[channel]);
        sliders.offsetSlider->setLimits(channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
        sliders.offsetSlider->setStep(channel, 0.2);
        sliders.offsetSlider->setValue(channel, scope->voltage[channel].offset);
        sliders.offsetSlider->setIndexVisible(channel, scope->voltage[channel].used);
    }
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        sliders.offsetSlider->addSlider(scope->spectrum[channel].name, scope->voltage.size() + channel);
        sliders.offsetSlider->setColor(scope->voltage.size() + channel, view->screen.spectrum[channel]);
        sliders.offsetSlider->setLimits(scope->voltage.size() + channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
        sliders.offsetSlider->setStep(scope->voltage.size() + channel, 0.2);
        sliders.offsetSlider->setValue(scope->voltage.size() + channel, scope->spectrum[channel].offset);
        sliders.offsetSlider->setIndexVisible(scope->voltage.size() + channel, scope->spectrum[channel].used);
    }

    // The triggerPosition slider
    sliders.triggerPositionSlider = new LevelSlider(Qt::DownArrow);
    sliders.triggerPositionSlider->addSlider();
    sliders.triggerPositionSlider->setLimits(0, 0.0, 1.0);
    sliders.triggerPositionSlider->setStep(0, 0.2 / (double)DIVS_TIME);
    sliders.triggerPositionSlider->setValue(0, scope->trigger.position);
    sliders.triggerPositionSlider->setIndexVisible(0, true);

    // The sliders for the trigger levels
    sliders.triggerLevelSlider = new LevelSlider(Qt::LeftArrow);
    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        sliders.triggerLevelSlider->addSlider((int)channel);
        sliders.triggerLevelSlider->setColor(channel,
                                             (channel == scope->trigger.source)
                                                 ? view->screen.voltage[channel]
                                                 : view->screen.voltage[channel].darker());
        adaptTriggerLevelSlider(sliders, channel);
        sliders.triggerLevelSlider->setValue(channel, scope->voltage[channel].trigger);
        sliders.triggerLevelSlider->setIndexVisible(channel, scope->voltage[channel].used);
    }

    // The marker slider
    sliders.markerSlider = new LevelSlider(Qt::UpArrow);
    for (int marker = 0; marker < MARKER_COUNT; ++marker) {
        sliders.markerSlider->addSlider(QString::number(marker + 1), marker);
        sliders.markerSlider->setLimits(marker, MARGIN_LEFT, MARGIN_RIGHT);
        sliders.markerSlider->setStep(marker, MARKER_STEP);
        sliders.markerSlider->setValue(marker, scope->horizontal.cursor.pos[marker].x());
        sliders.markerSlider->setIndexVisible(marker, true);
    }
}

/// \brief Set the trigger level sliders minimum and maximum to the new values.
void DsoWidget::adaptTriggerLevelSlider(DsoWidget::Sliders &sliders, ChannelID channel) {
    //printf( "DW::adaptTriggerLevelSlider( %d )\n", channel );
    sliders.triggerLevelSlider->setLimits((int)channel,
                                          (-DIVS_VOLTAGE / 2 - scope->voltage[channel].offset) * scope->gain(channel),
                                          ( DIVS_VOLTAGE / 2 - scope->voltage[channel].offset) * scope->gain(channel) );
    sliders.triggerLevelSlider->setStep((int)channel, scope->gain(channel) * 0.05);
    double value = sliders.triggerLevelSlider->value(channel); 
    if ( value  ) { // ignore when first called at init
        updateTriggerLevel(channel, value);
    }
}

/// \brief Show/Hide a line of the measurement table.
void DsoWidget::setMeasurementVisible(ChannelID channel) {
    bool visible = scope->voltage[channel].used || scope->spectrum[channel].used;

    measurementNameLabel[channel]->setVisible(visible);
    measurementMiscLabel[channel]->setVisible(visible);
    measurementGainLabel[channel]->setVisible(visible);
    measurementVppLabel[channel]->setVisible(visible);
    measurementRMSLabel[channel]->setVisible(visible);
    measurementDCLabel[channel]->setVisible(visible);
    measurementACLabel[channel]->setVisible(visible);
    measurementdBLabel[channel]->setVisible(visible);
    measurementFrequencyLabel[channel]->setVisible(visible);
    if (!visible) {
        measurementGainLabel[channel]->setText(QString());
        measurementVppLabel[channel]->setText(QString());
        measurementRMSLabel[channel]->setText(QString());
        measurementDCLabel[channel]->setText(QString());
        measurementACLabel[channel]->setText(QString());
        measurementdBLabel[channel]->setText(QString());
        measurementFrequencyLabel[channel]->setText(QString());
    }

    measurementGainLabel[channel]->setVisible(scope->voltage[channel].used);
    if (!scope->voltage[channel].used) { measurementGainLabel[channel]->setText(QString()); }

    measurementMagnitudeLabel[channel]->setVisible(scope->spectrum[channel].used);
    if (!scope->spectrum[channel].used) { measurementMagnitudeLabel[channel]->setText(QString()); }
}


/// \brief Update the label about the marker measurements
void DsoWidget::updateMarkerDetails() {
    double m1 = scope->horizontal.cursor.pos[0].x() + DIVS_TIME / 2; // zero at center -> zero at left margin
    double m2 = scope->horizontal.cursor.pos[1].x() + DIVS_TIME / 2; // zero at center -> zero at left margin
    if ( m1 > m2 )
        std::swap( m1, m2 );
    double divs = m2 - m1;
    // t = 0 at trigger position
    double time0 = ( m1 - DIVS_TIME * scope->trigger.position ) * scope->horizontal.timebase;
    double time1 = ( m2 - DIVS_TIME * scope->trigger.position ) * scope->horizontal.timebase;
    double time = divs * scope->horizontal.timebase;
    double freq0 = m1 * scope->horizontal.frequencybase;
    double freq1 = m2 * scope->horizontal.frequencybase;
    double freq = divs * scope->horizontal.frequencybase;
    bool timeUsed = false;
    bool freqUsed = false;

    int index = 0;
    cursorDataGrid->updateInfo(index++, true, QString(),
        valueToString(time, UNIT_SECONDS, 3), valueToString( freq, UNIT_HERTZ, 3 ) );

    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        if (scope->voltage[channel].used) {
            timeUsed = true; // at least one voltage channel used -> show marker time details
            QPointF p0 = scope->voltage[channel].cursor.pos[0];
            QPointF p1 = scope->voltage[channel].cursor.pos[1];
            cursorDataGrid->updateInfo(index, true,
                scope->voltage[channel].cursor.shape != DsoSettingsScopeCursor::NONE ? tr("ON") : tr("OFF"),
                valueToString(fabs(p1.x() - p0.x()) * scope->horizontal.timebase, UNIT_SECONDS, 4),
                valueToString(fabs(p1.y() - p0.y()) * scope->gain(channel), UNIT_VOLTS, 4));
        } else {
            cursorDataGrid->updateInfo(index, false);
        }
        ++index;
    }
    for (ChannelID channel = 0; channel < scope->spectrum.size(); ++channel) {
        if (scope->spectrum[channel].used) {
            freqUsed = true; // at least one spec channel used -> show marker freq details
            QPointF p0 = scope->spectrum[channel].cursor.pos[0];
            QPointF p1 = scope->spectrum[channel].cursor.pos[1];
            cursorDataGrid->updateInfo(index, true,
                scope->spectrum[channel].cursor.shape != DsoSettingsScopeCursor::NONE ? tr("ON") : tr("OFF"),
                valueToString(fabs(p1.x() - p0.x()) * scope->horizontal.frequencybase, UNIT_HERTZ, 4),
                valueToString(fabs(p1.y() - p0.y()) * scope->spectrum[channel].magnitude, UNIT_DECIBEL, 4));
        } else {
            cursorDataGrid->updateInfo(index, false);
        }
        ++index;
    }

    if ( DIVS_TIME == divs || ( m1 == 0 && m2 == 0) || ( m1 == DIVS_TIME && m2 == DIVS_TIME) ) {
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
        QString mInfo( tr( "Markers  ") );
        QString mTime( tr( "Time: ") );
        QString mFreq( tr( "Frequency: ") );
        if (view->zoom) {
            if ( divs != 0.0 )
                mInfo = tr( "Zoom x%L1  " ).arg( DIVS_TIME / divs, -1, 'g', 3 );
            else // avoid div by zero
                mInfo = tr( "Zoom ---  " );
            mTime = " t: ";
            mFreq = " f: ";
            markerTimebaseLabel->setText("  " + valueToString( time / DIVS_TIME, UNIT_SECONDS, 3 ) + tr("/div"));
            markerTimebaseLabel->setVisible( timeUsed );
            markerFrequencybaseLabel->setText( "  " + valueToString( freq / DIVS_TIME, UNIT_HERTZ, 3 ) + tr("/div"));
            markerFrequencybaseLabel->setVisible( freqUsed );
        }
        markerInfoLabel->setText( mInfo );
        if ( timeUsed ) {
            mTime += QString( "%1" ).arg( valueToString( time0, UNIT_SECONDS, 3 ) );
            if ( time )
                mTime += QString( " ... %1,  Δt: %2 (%3) ")
                    .arg( valueToString( time1, UNIT_SECONDS, 3 ) )
                    .arg( valueToString( time, UNIT_SECONDS, 3 ) )
                    .arg( valueToString( 1/time, UNIT_HERTZ, 3) )
                ;
            markerTimeLabel->setText( mTime );
        } else {
            markerTimeLabel->setText( "" );
        }
        if ( freqUsed ) {
             mFreq += QString( "%1" ).arg( valueToString( freq0, UNIT_HERTZ, 3 ) );
            if ( freq )
                mFreq += QString( " ... %2,  Δf: %3 " )
                    .arg( valueToString( freq1, UNIT_HERTZ, 3 ) )
                    .arg( valueToString( freq, UNIT_HERTZ, 3 ) )
                ;
            markerFrequencyLabel->setText( mFreq );
        } else
            markerFrequencyLabel->setText( "" );
    }
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateSpectrumDetails(ChannelID channel) {
    setMeasurementVisible(channel);

    if (scope->spectrum[channel].used)
        measurementMagnitudeLabel[channel]->setText(valueToString(scope->spectrum[channel].magnitude, UNIT_DECIBEL, 3) +
                                                    tr("/div"));
    else
        measurementMagnitudeLabel[channel]->setText(QString());
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateTriggerDetails() {
    // Update the trigger details
    QPalette tablePalette = palette();
    tablePalette.setColor(QPalette::WindowText, view->screen.voltage[scope->trigger.source]);
    settingsTriggerLabel->setPalette(tablePalette);
    QString levelString = valueToString(scope->voltage[scope->trigger.source].trigger, UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg( (int)round(scope->trigger.position * 100 ) );
    QString pre = Dso::slopeString(scope->trigger.slope); // trigger slope
    QString post = pre; // opposite trigger slope
    if ( scope->trigger.slope == Dso::Slope::Positive )
        post = Dso::slopeString( Dso::Slope:: Negative );
    else if ( scope->trigger.slope == Dso::Slope::Negative )
        post = Dso::slopeString( Dso::Slope:: Positive );
    QString pulseWidthString = pulseWidth1 ? pre + valueToString( pulseWidth1, UNIT_SECONDS, 3) + post : "";
    pulseWidthString += pulseWidth2 ? valueToString( pulseWidth2, UNIT_SECONDS, 3) + pre : "";
    if ( pulseWidth1 && pulseWidth2 ) {
        int dutyCyle = 0.5 + ( 100.0 * pulseWidth1 ) / (pulseWidth1 + pulseWidth2);
        pulseWidthString += " (" + QString::number( dutyCyle ) + "%)";
    }
    settingsTriggerLabel->setText( tr( "%1  %2  %3  %4  %5" )
                                      .arg( scope->voltage[scope->trigger.source].name,
                                            Dso::slopeString(scope->trigger.slope),
                                            levelString, pretriggerString, pulseWidthString
                                          )
                                 );
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateVoltageDetails(ChannelID channel) {
    if (channel >= scope->voltage.size()) return;

    setMeasurementVisible(channel);

    if (scope->voltage[channel].used)
        measurementGainLabel[channel]->setText(valueToString(scope->gain(channel), UNIT_VOLTS, 3) + tr("/div"));
    else
        measurementGainLabel[channel]->setText(QString());
}

/// \brief Handles frequencybaseChanged signal from the horizontal dock.
/// \param frequencybase The frequencybase used for displaying the trace.
void DsoWidget::updateFrequencybase(double frequencybase) {
    settingsFrequencybaseLabel->setText(valueToString(frequencybase, UNIT_HERTZ, -1) + tr("/div"));
    updateMarkerDetails();
}

/// \brief Updates the samplerate field after changing the samplerate.
/// \param samplerate The samplerate set in the oscilloscope.
void DsoWidget::updateSamplerate(double samplerate) {
    this->samplerate = samplerate;
    dotsOnScreen = samplerate * timebase * DIVS_TIME + 0.99;
    //printf( "DsoWidget::updateSamplerate( %g ) -> %d\n", samplerate, dotsOnScreen );
    settingsSamplerateLabel->setText(valueToString(samplerate, UNIT_SAMPLES, -1) + tr("/s"));
}

/// \brief Handles timebaseChanged signal from the horizontal dock.
/// \param timebase The timebase used for displaying the trace.
void DsoWidget::updateTimebase(double timebase) {
    this->timebase = timebase;
    dotsOnScreen = samplerate * timebase * DIVS_TIME + 0.99;
    //printf( "DsoWidget::updateTimebase( %g ) -> %d\n", timebase, dotsOnScreen );
    settingsTimebaseLabel->setText(valueToString(timebase, UNIT_SECONDS, -1) + tr("/div"));
    updateMarkerDetails();
}

/// \brief Handles magnitudeChanged signal from the spectrum dock.
/// \param channel The channel whose magnitude was changed.
void DsoWidget::updateSpectrumMagnitude(ChannelID channel) { updateSpectrumDetails(channel); }

/// \brief Handles usedChanged signal from the spectrum dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateSpectrumUsed(ChannelID channel, bool used) {
    if (channel >= (unsigned int)scope->voltage.size()) return;

//    if (!used && cursorDataGrid->spectrumCursors[channel].selector->isChecked()) cursorDataGrid->selectItem(0);

    mainSliders.offsetSlider->setIndexVisible(scope->voltage.size() + channel, used);
    zoomSliders.offsetSlider->setIndexVisible(scope->voltage.size() + channel, used);

    updateSpectrumDetails(channel);
    updateMarkerDetails();
}

/// \brief Handles modeChanged signal from the trigger dock.
void DsoWidget::updateTriggerMode() { updateTriggerDetails(); }

/// \brief Handles slopeChanged signal from the trigger dock.
void DsoWidget::updateTriggerSlope() { updateTriggerDetails(); }

/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
    // Change the colors of the trigger sliders
    mainSliders.triggerPositionSlider->setColor(0, view->screen.voltage[scope->trigger.source]);
    zoomSliders.triggerPositionSlider->setColor(0, view->screen.voltage[scope->trigger.source]);

    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        QColor color = (channel == scope->trigger.source)
                           ? view->screen.voltage[channel]
                           : view->screen.voltage[channel].darker();
        mainSliders.triggerLevelSlider->setColor(channel, color);
        zoomSliders.triggerLevelSlider->setColor(channel, color);
    }

    updateTriggerDetails();
}

/// \brief Handles couplingChanged signal from the voltage dock.
/// \param channel The channel whose coupling was changed.
void DsoWidget::updateVoltageCoupling(ChannelID channel) {
    if (channel >= (unsigned int)scope->voltage.size())
        return;
    measurementMiscLabel[channel]->setText(Dso::couplingString(scope->coupling(channel, spec)));
}

/// \brief Handles modeChanged signal from the voltage dock.
void DsoWidget::updateMathMode() {
    measurementMiscLabel[spec->channels]->setText(
        Dso::mathModeString(Dso::getMathMode(scope->voltage[spec->channels])));
}

/// \brief Handles gainChanged signal from the voltage dock.
/// \param channel The channel whose gain was changed.
void DsoWidget::updateVoltageGain(ChannelID channel) {
    if (channel >= (unsigned int)scope->voltage.size())
        return;
    if (channel < spec->channels) {
        adaptTriggerLevelSlider(mainSliders, channel);
        adaptTriggerLevelSlider(zoomSliders, channel);
    }
    updateVoltageDetails(channel);
}

/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateVoltageUsed(ChannelID channel, bool used) {
    if (channel >= (unsigned int)scope->voltage.size()) return;

//    if (!used && cursorDataGrid->voltageCursors[channel].selector->isChecked()) cursorDataGrid->selectItem(0);

    mainSliders.offsetSlider->setIndexVisible(channel, used);
    zoomSliders.offsetSlider->setIndexVisible(channel, used);

    mainSliders.triggerLevelSlider->setIndexVisible(channel, used);
    zoomSliders.triggerLevelSlider->setIndexVisible(channel, used);

    setMeasurementVisible(channel);
    updateVoltageDetails(channel);
    updateMarkerDetails();
}

/// \brief Change the record length.
void DsoWidget::updateRecordLength(unsigned long size) {
    settingsSamplesOnScreen->setText(valueToString(size, UNIT_SAMPLES, -1) + tr(" on screen") );
}

/// \brief Show/hide the zoom view.
void DsoWidget::updateZoom(bool enabled) {
    mainLayout->setRowStretch(zoomScopeRow, enabled ? 1 : 0);
    zoomScope->setVisible(enabled);

    if (enabled) {
        zoomSliders.offsetSlider->show();
        zoomSliders.triggerPositionSlider->show();
        zoomSliders.triggerLevelSlider->show();
    } else {
        zoomSliders.offsetSlider->hide();
        zoomSliders.triggerPositionSlider->hide();
        zoomSliders.triggerLevelSlider->hide();
    }

    // Show time-/frequencybase and zoom factor if the magnified scope is shown
    markerLayout->setStretch(3, enabled ? 1 : 0);
    markerTimebaseLabel->setVisible(enabled);
    markerLayout->setStretch(4, enabled ? 1 : 0);
    markerFrequencybaseLabel->setVisible(enabled);
    updateMarkerDetails();

    repaint();
}

/// \brief Prints analyzed data.
void DsoWidget::showNew(std::shared_ptr<PPresult> data) {
    mainScope->showData(data);
    zoomScope->showData(data);

    QPalette triggerLabelPalette = palette();
    triggerLabelPalette.setColor(QPalette::WindowText, Qt::black);
    triggerLabelPalette.setColor(QPalette::Background, data->softwareTriggerTriggered ? Qt::green : Qt::red);
    swTriggerStatus->setPalette(triggerLabelPalette);
    swTriggerStatus->setVisible(true);
    updateRecordLength(dotsOnScreen);
    pulseWidth1 = data.get()->data( 0 )->pulseWidth1;
    pulseWidth2 = data.get()->data( 0 )->pulseWidth2;
    updateTriggerDetails();
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        if (scope->voltage[channel].used && data.get()->data(channel)) {
            // Vpp Amplitude string representation (3 significant digits)
            measurementVppLabel[channel]->setText(
                valueToString( data.get()->data(channel)->vpp, UNIT_VOLTS, 3 ) + "pp" );
            // RMS Amplitude string representation (3 significant digits)
            measurementRMSLabel[channel]->setText(
                valueToString( data.get()->data(channel)->rms, UNIT_VOLTS, 3 ) + "rms" );
            // DC Amplitude string representation (3 significant digits)
            measurementDCLabel[channel]->setText(
                valueToString( data.get()->data(channel)->dc, UNIT_VOLTS, 3 ) + "=" );
            // AC Amplitude string representation (3 significant digits)
            measurementACLabel[channel]->setText(
                valueToString( data.get()->data(channel)->ac, UNIT_VOLTS, 3 ) + "~" );
            // dB Amplitude string representation (3 significant digits)
            measurementdBLabel[channel]->setText(
                valueToString( data.get()->data(channel)->dB, UNIT_DECIBEL, 3 ) );
            // Frequency string representation (3 significant digits)
            measurementFrequencyLabel[channel]->setText(
                valueToString( data.get()->data(channel)->frequency, UNIT_HERTZ, 4 ) );
            // Highlight clipped channel
            QPalette validPalette;
            if ( data.get()->data(channel)->valid ) { // normal display
                validPalette.setColor( QPalette::WindowText, view->screen.voltage[channel] );
                validPalette.setColor( QPalette::Background, view->screen.background );
            } else { // warning
                validPalette.setColor(QPalette::WindowText, Qt::black );
                validPalette.setColor(QPalette::Background, Qt::red );
            }
            measurementNameLabel[channel]->setPalette( validPalette );
        }
    }
}

void DsoWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    // Apply settings and update measured values
    updateTriggerDetails();
    updateRecordLength(scope->horizontal.recordLength);
    updateFrequencybase(scope->horizontal.frequencybase);
    updateSamplerate(scope->horizontal.samplerate);
    updateTimebase(scope->horizontal.timebase);
    updateZoom(view->zoom);

    updateTriggerSource();
    adaptTriggerPositionSlider();
}

/// \brief Handles valueChanged signal from the offset sliders.
/// \param channel The channel whose offset was changed.
/// \param value The new offset for the channel.
void DsoWidget::updateOffset(ChannelID channel, double value) {
    if (channel < scope->voltage.size()) {
        scope->voltage[channel].offset = value;

        if (channel < spec->channels) {
            adaptTriggerLevelSlider(mainSliders, channel);
            adaptTriggerLevelSlider(zoomSliders, channel);
        }
    } else if (channel < scope->voltage.size() * 2)
        scope->spectrum[channel - scope->voltage.size()].offset = value;

    if (channel < scope->voltage.size() * 2) {
        if (mainSliders.offsetSlider->value(channel) != value) {
            const QSignalBlocker blocker(mainSliders.offsetSlider);
            mainSliders.offsetSlider->setValue(channel, value);
        }
        if (zoomSliders.offsetSlider->value(channel) != value) {
            const QSignalBlocker blocker(zoomSliders.offsetSlider);
            zoomSliders.offsetSlider->setValue(channel, value);
        }
    }

    emit offsetChanged(channel, value);
}

/// \brief Translate horizontal position (0..1) from main view to zoom view.
double DsoWidget::mainToZoom(double position) const {
    double m1 = scope->getMarker(0);
    double m2 = scope->getMarker(1);
    if ( m1 == m2 )
        m2 += 1e-9; // avoid div by zero
    if (m1 > m2) std::swap(m1, m2);
    return ((position - 0.5) * DIVS_TIME - m1) / (m2 - m1);
}

/// \brief Translate horizontal position (0..1) from zoom view to main view.
double DsoWidget::zoomToMain(double position) const {
    double m1 = scope->getMarker(0);
    double m2 = scope->getMarker(1);
    if (m1 > m2) std::swap(m1, m2);
    return 0.5 + (m1 + position * (m2 - m1)) / DIVS_TIME;
}

/// \brief Handles signals affecting trigger position in the zoom view.
void DsoWidget::adaptTriggerPositionSlider() {
    double value = mainToZoom(scope->trigger.position);

    LevelSlider &slider = *zoomSliders.triggerPositionSlider;
    const QSignalBlocker blocker(slider);
    if (slider.minimum(0) <= value && value <= slider.maximum(0)) {
        slider.setEnabled(true);
        slider.setValue(0, value);
    } else {
        slider.setEnabled(false);
        if (value < slider.minimum(0)) {
            slider.setValue(0, slider.minimum(0));
        } else {
            slider.setValue(0, slider.maximum(0));
        }
    }
}

/// \brief Handles valueChanged signal from the triggerPosition slider.
/// \param index The index of the slider.
/// \param value The new triggerPosition in seconds relative to the first
/// sample.
void DsoWidget::updateTriggerPosition(int index, double value, bool mainView) {
    if (index != 0) return;

    if (mainView) {
        scope->trigger.position = value;
        adaptTriggerPositionSlider();
    } else {
        scope->trigger.position = zoomToMain(value);
        const QSignalBlocker blocker(mainSliders.triggerPositionSlider);
        mainSliders.triggerPositionSlider->setValue(index, scope->trigger.position);
    }

    updateTriggerDetails();
    updateMarkerDetails();

    emit triggerPositionChanged(scope->trigger.position);
}

/// \brief Handles valueChanged signal from the trigger level slider.
/// \param channel The index of the slider.
/// \param value The new trigger level.
void DsoWidget::updateTriggerLevel(ChannelID channel, double value) {
    //printf("DW::updateTriggerLevel( %d, %g )\n", channel, value);
    scope->voltage[channel].trigger = value;

    if (mainSliders.triggerLevelSlider->value(channel) != value) {
        const QSignalBlocker blocker(mainSliders.triggerLevelSlider);
        mainSliders.triggerLevelSlider->setValue(channel, value);
    }
    if (zoomSliders.triggerLevelSlider->value(channel) != value) {
        const QSignalBlocker blocker(zoomSliders.triggerLevelSlider);
        zoomSliders.triggerLevelSlider->setValue(channel, value);
    }

    updateTriggerDetails();

    emit triggerLevelChanged(channel, value);
}

/// \brief Handles valueChanged signal from the marker slider.
/// \param marker The index of the slider.
/// \param value The new marker position.
void DsoWidget::updateMarker(int marker, double value) {
    scope->setMarker(marker, value);
    adaptTriggerPositionSlider();
    updateMarkerDetails();
}
