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

    connect(mainScope, &GlScope::markerMoved, [this](int marker, double position) {
        double value = std::round(position / MARKER_STEP) * MARKER_STEP;

        this->scope->horizontal.marker[marker] = value;
        this->mainSliders.markerSlider->setValue(marker, value);
        this->mainScope->markerUpdated();
    });

    // The table for the settings
    settingsTriggerLabel = new QLabel();
    settingsTriggerLabel->setMinimumWidth(160);
    settingsTriggerLabel->setIndent(5);
    settingsRecordLengthLabel = new QLabel();
    settingsRecordLengthLabel->setAlignment(Qt::AlignRight);
    settingsRecordLengthLabel->setPalette(palette);
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
    swTriggerStatus->setMinimumWidth(30);
    swTriggerStatus->setText(tr("TR"));
    swTriggerStatus->setAlignment(Qt::AlignCenter);
    swTriggerStatus->setAutoFillBackground(true);
    swTriggerStatus->setVisible(false);
    settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget(swTriggerStatus);
    settingsLayout->addWidget(settingsTriggerLabel);
    settingsLayout->addWidget(settingsRecordLengthLabel, 1);
    settingsLayout->addWidget(settingsSamplerateLabel, 1);
    settingsLayout->addWidget(settingsTimebaseLabel, 1);
    settingsLayout->addWidget(settingsFrequencybaseLabel, 1);

    // The table for the marker details
    markerInfoLabel = new QLabel();
    markerInfoLabel->setMinimumWidth(160);
    markerInfoLabel->setPalette(palette);
    markerTimeLabel = new QLabel();
    markerTimeLabel->setAlignment(Qt::AlignRight);
    markerTimeLabel->setPalette(palette);
    markerFrequencyLabel = new QLabel();
    markerFrequencyLabel->setAlignment(Qt::AlignRight);
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

    // The table for the measurements
    QPalette tablePalette = palette;
    measurementLayout = new QGridLayout();
    measurementLayout->setColumnMinimumWidth(0, 64);
    measurementLayout->setColumnMinimumWidth(1, 32);
    measurementLayout->setColumnStretch(2, 2);
    measurementLayout->setColumnStretch(3, 2);
    measurementLayout->setColumnStretch(4, 3);
    measurementLayout->setColumnStretch(5, 3);
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        tablePalette.setColor(QPalette::WindowText, view->screen.voltage[channel]);
        measurementNameLabel.push_back(new QLabel(scope->voltage[channel].name));
        measurementNameLabel[channel]->setPalette(tablePalette);
        measurementMiscLabel.push_back(new QLabel());
        measurementMiscLabel[channel]->setPalette(tablePalette);
        measurementGainLabel.push_back(new QLabel());
        measurementGainLabel[channel]->setAlignment(Qt::AlignRight);
        measurementGainLabel[channel]->setPalette(tablePalette);
        tablePalette.setColor(QPalette::WindowText, view->screen.spectrum[channel]);
        measurementMagnitudeLabel.push_back(new QLabel());
        measurementMagnitudeLabel[channel]->setAlignment(Qt::AlignRight);
        measurementMagnitudeLabel[channel]->setPalette(tablePalette);
        measurementAmplitudeLabel.push_back(new QLabel());
        measurementAmplitudeLabel[channel]->setAlignment(Qt::AlignRight);
        measurementAmplitudeLabel[channel]->setPalette(palette);
        measurementFrequencyLabel.push_back(new QLabel());
        measurementFrequencyLabel[channel]->setAlignment(Qt::AlignRight);
        measurementFrequencyLabel[channel]->setPalette(palette);
        setMeasurementVisible(channel);
        measurementLayout->addWidget(measurementNameLabel[channel], (int)channel, 0);
        measurementLayout->addWidget(measurementMiscLabel[channel], (int)channel, 1);
        measurementLayout->addWidget(measurementGainLabel[channel], (int)channel, 2);
        measurementLayout->addWidget(measurementMagnitudeLabel[channel], (int)channel, 3);
        measurementLayout->addWidget(measurementAmplitudeLabel[channel], (int)channel, 4);
        measurementLayout->addWidget(measurementFrequencyLabel[channel], (int)channel, 5);
        if ((unsigned)channel < spec->channels)
            updateVoltageCoupling((unsigned)channel);
        else
            updateMathMode();
        updateVoltageDetails((unsigned)channel);
        updateSpectrumDetails((unsigned)channel);
    }

    // The layout for the widgets
    mainLayout = new QGridLayout();
    mainLayout->setColumnStretch(2, 1); // Scopes increase their size
    // Bars around the scope, needed because the slider-drawing-area is outside
    // the scope at min/max
    mainLayout->setColumnMinimumWidth(1, mainSliders.triggerPositionSlider->preMargin());
    mainLayout->setColumnMinimumWidth(3, mainSliders.triggerPositionSlider->postMargin());
    mainLayout->setSpacing(0);
    int row = 0;
    mainLayout->addLayout(settingsLayout, row++, 0, 1, 5);
    // 5x5 box for mainScope & mainSliders
    mainLayout->setRowMinimumHeight(row + 1, mainSliders.offsetSlider->preMargin());
    mainLayout->setRowMinimumHeight(row + 3, mainSliders.offsetSlider->postMargin());
    mainLayout->setRowStretch(row + 2, 1);
    mainLayout->addWidget(mainScope, row + 2, 2);
    mainLayout->addWidget(mainSliders.offsetSlider, row + 1, 0, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(mainSliders.triggerPositionSlider, row, 1, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(mainSliders.triggerLevelSlider, row + 1, 3, 3, 2, Qt::AlignLeft);
    mainLayout->addWidget(mainSliders.markerSlider, row + 3, 1, 2, 3, Qt::AlignTop);
    row += 5;
    // Separators and markerLayout
    mainLayout->setRowMinimumHeight(row++, 4);
    mainLayout->addLayout(markerLayout, row++, 0, 1, 5);
    mainLayout->setRowMinimumHeight(row++, 4);
    // 5x5 box for zoomScope & zoomSliders
    zoomScopeRow = row + 2;
    mainLayout->addWidget(zoomScope, zoomScopeRow, 2);
    mainLayout->addWidget(zoomSliders.offsetSlider, row + 1, 0, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(zoomSliders.triggerPositionSlider, row, 1, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(zoomSliders.triggerLevelSlider, row + 1, 3, 3, 2, Qt::AlignLeft);
    row += 5;
    // Separator and embedded measurementLayout
    mainLayout->setRowMinimumHeight(row++, 8);
    mainLayout->addLayout(measurementLayout, row++, 0, 1, 5);

    // The widget itself
    setPalette(palette);
    setBackgroundRole(QPalette::Background);
    setAutoFillBackground(true);
    setLayout(mainLayout);

    // Connect change-signals of sliders
    connect(mainSliders.offsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset);
    connect(zoomSliders.offsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset);

    connect(mainSliders.triggerPositionSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerPosition);
    zoomSliders.triggerPositionSlider->setEnabled(false);

    connect(mainSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel);
    connect(zoomSliders.triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel);

    connect(mainSliders.markerSlider, &LevelSlider::valueChanged, [this](int index, double value) {
        updateMarker(index, value);
        mainScope->update();
        zoomScope->update();
    });
    zoomSliders.markerSlider->setEnabled(false);
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
                                             (!scope->trigger.special && channel == scope->trigger.source)
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
        sliders.markerSlider->setLimits(marker, -DIVS_TIME / 2, DIVS_TIME / 2);
        sliders.markerSlider->setStep(marker, MARKER_STEP);
        sliders.markerSlider->setValue(marker, scope->horizontal.marker[marker]);
        sliders.markerSlider->setIndexVisible(marker, true);
    }
}

/// \brief Set the trigger level sliders minimum and maximum to the new values.
void DsoWidget::adaptTriggerLevelSlider(DsoWidget::Sliders &sliders, ChannelID channel) {
    sliders.triggerLevelSlider->setLimits((int)channel,
                                          (-DIVS_VOLTAGE / 2 - scope->voltage[channel].offset) * scope->gain(channel),
                                          (DIVS_VOLTAGE / 2 - scope->voltage[channel].offset) * scope->gain(channel));
    sliders.triggerLevelSlider->setStep((int)channel, scope->gain(channel) * 0.05);
}

/// \brief Show/Hide a line of the measurement table.
void DsoWidget::setMeasurementVisible(ChannelID channel) {

    bool visible = scope->voltage[channel].used || scope->spectrum[channel].used;

    measurementNameLabel[channel]->setVisible(visible);
    measurementMiscLabel[channel]->setVisible(visible);

    measurementAmplitudeLabel[channel]->setVisible(visible);
    measurementFrequencyLabel[channel]->setVisible(visible);
    if (!visible) {
        measurementGainLabel[channel]->setText(QString());
        measurementAmplitudeLabel[channel]->setText(QString());
        measurementFrequencyLabel[channel]->setText(QString());
    }

    measurementGainLabel[channel]->setVisible(scope->voltage[channel].used);
    if (!scope->voltage[channel].used) { measurementGainLabel[channel]->setText(QString()); }

    measurementMagnitudeLabel[channel]->setVisible(scope->spectrum[channel].used);
    if (!scope->spectrum[channel].used) { measurementMagnitudeLabel[channel]->setText(QString()); }
}

static QString markerToString(DsoSettingsScope *scope, unsigned index) {
    double value = (DIVS_TIME * (0.5 - scope->trigger.position) + scope->horizontal.marker[index]) * scope->horizontal.timebase;
    int precision = 3 - (int)floor(log10(fabs(value)));

    if (scope->horizontal.timebase < 1e-9)
        return QApplication::tr("%L1 ps").arg(value / 1e-12, 0, 'f', qBound(0, precision - 12, 3));
    else if (scope->horizontal.timebase < 1e-6)
        return QApplication::tr("%L1 ns").arg(value / 1e-9, 0, 'f', qBound(0, precision - 9, 3));
    else if (scope->horizontal.timebase < 1e-3)
        return QApplication::tr("%L1 µs").arg(value / 1e-6, 0, 'f', qBound(0, precision - 6, 3));
    else if (scope->horizontal.timebase < 1)
        return QApplication::tr("%L1 ms").arg(value / 1e-3, 0, 'f', qBound(0, precision - 3, 3));
    else
        return QApplication::tr("%L1 s").arg(value, 0, 'f', qBound(0, precision, 3));
}

/// \brief Update the label about the marker measurements
void DsoWidget::updateMarkerDetails() {
    double divs = fabs(scope->horizontal.marker[1] - scope->horizontal.marker[0]);
    double time = divs * scope->horizontal.timebase;

    QString prefix(tr("Markers"));
    if (view->zoom) {
        prefix = tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3);
        markerTimebaseLabel->setText(valueToString(time / DIVS_TIME, UNIT_SECONDS, 3) + tr("/div"));
        markerFrequencybaseLabel->setText(
            valueToString(divs * scope->horizontal.frequencybase / DIVS_TIME, UNIT_HERTZ, 4) + tr("/div"));
    }
    markerInfoLabel->setText(prefix.append(":  %1  %2").arg(markerToString(scope, 0)).arg(markerToString(scope, 1)));
    markerTimeLabel->setText(valueToString(time, UNIT_SECONDS, 4));
    markerFrequencyLabel->setText(valueToString(1.0 / time, UNIT_HERTZ, 4));
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
    QString pretriggerString = tr("%L1%").arg((int)(scope->trigger.position * 100 + 0.5));
    settingsTriggerLabel->setText(tr("%1  %2  %3  %4")
                                      .arg(scope->voltage[scope->trigger.source].name,
                                           Dso::slopeString(scope->trigger.slope), levelString, pretriggerString));

    /// \todo This won't work for special trigger sources
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
    settingsFrequencybaseLabel->setText(valueToString(frequencybase, UNIT_HERTZ, 4) + tr("/div"));
}

/// \brief Updates the samplerate field after changing the samplerate.
/// \param samplerate The samplerate set in the oscilloscope.
void DsoWidget::updateSamplerate(double samplerate) {
    settingsSamplerateLabel->setText(valueToString(samplerate, UNIT_SAMPLES, 4) + tr("/s"));
}

/// \brief Handles timebaseChanged signal from the horizontal dock.
/// \param timebase The timebase used for displaying the trace.
void DsoWidget::updateTimebase(double timebase) {
    settingsTimebaseLabel->setText(valueToString(timebase, UNIT_SECONDS, 4) + tr("/div"));

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

    mainSliders.offsetSlider->setIndexVisible(scope->voltage.size() + channel, used);
    zoomSliders.offsetSlider->setIndexVisible(scope->voltage.size() + channel, used);

    updateSpectrumDetails(channel);
}

/// \brief Handles modeChanged signal from the trigger dock.
void DsoWidget::updateTriggerMode() { updateTriggerDetails(); }

/// \brief Handles slopeChanged signal from the trigger dock.
void DsoWidget::updateTriggerSlope() { updateTriggerDetails(); }

/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
    // Change the colors of the trigger sliders
    if (scope->trigger.special || scope->trigger.source >= spec->channels) {
        mainSliders.triggerPositionSlider->setColor(0, view->screen.border);
        zoomSliders.triggerPositionSlider->setColor(0, view->screen.border);
    } else {
        mainSliders.triggerPositionSlider->setColor(0, view->screen.voltage[scope->trigger.source]);
        zoomSliders.triggerPositionSlider->setColor(0, view->screen.voltage[scope->trigger.source]);
    }

    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        QColor color = (!scope->trigger.special && channel == scope->trigger.source)
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
    if (channel >= (unsigned int)scope->voltage.size()) return;

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
    if (channel >= (unsigned int)scope->voltage.size()) return;

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

    mainSliders.offsetSlider->setIndexVisible(channel, used);
    zoomSliders.offsetSlider->setIndexVisible(channel, used);

    mainSliders.triggerLevelSlider->setIndexVisible(channel, used);
    zoomSliders.triggerLevelSlider->setIndexVisible(channel, used);

    setMeasurementVisible(channel);
    updateVoltageDetails(channel);
}

/// \brief Change the record length.
void DsoWidget::updateRecordLength(unsigned long size) {
    settingsRecordLengthLabel->setText(valueToString(size, UNIT_SAMPLES, 4));
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

    if (spec->isSoftwareTriggerDevice) {
        QPalette triggerLabelPalette = palette();
        triggerLabelPalette.setColor(QPalette::WindowText, Qt::black);
        triggerLabelPalette.setColor(QPalette::Background, data->softwareTriggerTriggered ? Qt::green : Qt::red);
        swTriggerStatus->setPalette(triggerLabelPalette);
        swTriggerStatus->setVisible(true);
    }

    updateRecordLength(data.get()->sampleCount());

    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        if (scope->voltage[channel].used && data.get()->data(channel)) {
            // Amplitude string representation (4 significant digits)
            measurementAmplitudeLabel[channel]->setText(
                valueToString(data.get()->data(channel)->computeAmplitude(), UNIT_VOLTS, 4));
            // Frequency string representation (5 significant digits)
            measurementFrequencyLabel[channel]->setText(
                valueToString(data.get()->data(channel)->frequency, UNIT_HERTZ, 5));
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
            QSignalBlocker blocker(mainSliders.offsetSlider);
            mainSliders.offsetSlider->setValue(channel, value);
        }
        if (zoomSliders.offsetSlider->value(channel) != value) {
            QSignalBlocker blocker(zoomSliders.offsetSlider);
            zoomSliders.offsetSlider->setValue(channel, value);
        }
    }

    emit offsetChanged(channel, value);
}

/// \brief Handles signals affecting trigger position in the zoom view.
void DsoWidget::adaptTriggerPositionSlider() {
    double m1 = scope->horizontal.marker[0];
    double m2 = scope->horizontal.marker[1];

    if (m1 > m2) std::swap(m1, m2);
    double value = (scope->trigger.position - 0.5) * DIVS_TIME;
    if (m1 != m2 && m1 <= value && value <= m2) {
        zoomSliders.triggerPositionSlider->setIndexVisible(0, true);
        zoomSliders.triggerPositionSlider->setValue(0, (value - m1) / (m2 - m1));
    } else {
        zoomSliders.triggerPositionSlider->setIndexVisible(0, false);
    }
}

/// \brief Handles valueChanged signal from the triggerPosition slider.
/// \param index The index of the slider.
/// \param value The new triggerPosition in seconds relative to the first
/// sample.
void DsoWidget::updateTriggerPosition(int index, double value) {
    if (index != 0) return;

    scope->trigger.position = value;
    adaptTriggerPositionSlider();

    updateTriggerDetails();
    updateMarkerDetails();

    emit triggerPositionChanged(value * scope->horizontal.timebase * DIVS_TIME);
}

/// \brief Handles valueChanged signal from the trigger level slider.
/// \param channel The index of the slider.
/// \param value The new trigger level.
void DsoWidget::updateTriggerLevel(ChannelID channel, double value) {
    scope->voltage[channel].trigger = value;

    if (mainSliders.triggerLevelSlider->value(channel) != value) {
        QSignalBlocker blocker(mainSliders.triggerLevelSlider);
        mainSliders.triggerLevelSlider->setValue(channel, value);
    }
    if (zoomSliders.triggerLevelSlider->value(channel) != value) {
        QSignalBlocker blocker(zoomSliders.triggerLevelSlider);
        zoomSliders.triggerLevelSlider->setValue(channel, value);
    }

    updateTriggerDetails();

    emit triggerLevelChanged(channel, value);
}

/// \brief Handles valueChanged signal from the marker slider.
/// \param marker The index of the slider.
/// \param value The new marker position.
void DsoWidget::updateMarker(int marker, double value) {
    scope->horizontal.marker[marker] = value;

    adaptTriggerPositionSlider();
    updateMarkerDetails();

    emit markerChanged(marker, value);
}
