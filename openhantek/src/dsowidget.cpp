// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>

#include "dsowidget.h"

#include "exporter.h"
#include "glgenerator.h"
#include "glscope.h"
#include "settings.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"
#include "widgets/levelslider.h"

DsoWidget::DsoWidget(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags), settings(settings), generator(new GlGenerator(&settings->scope, &settings->view)),
      mainScope(new GlScope(settings, generator)), zoomScope(new GlScope(settings, generator)) {

    // Palette for this widget
    QPalette palette;
    palette.setColor(QPalette::Background, settings->view.screen.background);
    palette.setColor(QPalette::WindowText, settings->view.screen.text);

    // The OpenGL accelerated scope widgets
    zoomScope->setZoomMode(true);

    // The offset sliders for all possible channels
    offsetSlider = new LevelSlider(Qt::RightArrow);
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        offsetSlider->addSlider(settings->scope.voltage[channel].name, channel);
        offsetSlider->setColor(channel, settings->view.screen.voltage[channel]);
        offsetSlider->setLimits(channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
        offsetSlider->setStep(channel, 0.2);
        offsetSlider->setValue(channel, settings->scope.voltage[channel].offset);
        offsetSlider->setVisible(channel, settings->scope.voltage[channel].used);
    }
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        offsetSlider->addSlider(settings->scope.spectrum[channel].name, settings->scope.voltage.count() + channel);
        offsetSlider->setColor(settings->scope.voltage.count() + channel, settings->view.screen.spectrum[channel]);
        offsetSlider->setLimits(settings->scope.voltage.count() + channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
        offsetSlider->setStep(settings->scope.voltage.count() + channel, 0.2);
        offsetSlider->setValue(settings->scope.voltage.count() + channel, settings->scope.spectrum[channel].offset);
        offsetSlider->setVisible(settings->scope.voltage.count() + channel, settings->scope.spectrum[channel].used);
    }

    // The triggerPosition slider
    triggerPositionSlider = new LevelSlider(Qt::DownArrow);
    triggerPositionSlider->addSlider();
    triggerPositionSlider->setLimits(0, 0.0, 1.0);
    triggerPositionSlider->setStep(0, 0.2 / DIVS_TIME);
    triggerPositionSlider->setValue(0, settings->scope.trigger.position);
    triggerPositionSlider->setVisible(0, true);

    // The sliders for the trigger levels
    triggerLevelSlider = new LevelSlider(Qt::LeftArrow);
    for (int channel = 0; channel < (int)settings->scope.physicalChannels; ++channel) {
        triggerLevelSlider->addSlider(channel);
        triggerLevelSlider->setColor(
            channel,
            (!settings->scope.trigger.special && channel == (int)settings->scope.trigger.source)
                ? settings->view.screen.voltage[channel]
                : settings->view.screen.voltage[channel].darker());
        adaptTriggerLevelSlider(channel);
        triggerLevelSlider->setValue(channel, settings->scope.voltage[channel].trigger);
        triggerLevelSlider->setVisible(channel, settings->scope.voltage[channel].used);
    }

    // The marker slider
    markerSlider = new LevelSlider(Qt::UpArrow);
    for (int marker = 0; marker < MARKER_COUNT; ++marker) {
        markerSlider->addSlider(QString::number(marker + 1), marker);
        markerSlider->setLimits(marker, -DIVS_TIME / 2, DIVS_TIME / 2);
        markerSlider->setStep(marker, 0.2);
        markerSlider->setValue(marker, settings->scope.horizontal.marker[marker]);
        markerSlider->setVisible(marker, true);
        settings->scope.horizontal.marker_visible[marker] = true;
    }

    // The table for the settings
    settingsTriggerLabel = new QLabel();
    settingsTriggerLabel->setMinimumWidth(160);
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
    settingsLayout = new QHBoxLayout();
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
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        tablePalette.setColor(QPalette::WindowText, settings->view.screen.voltage[channel]);
        measurementNameLabel.append(new QLabel(settings->scope.voltage[channel].name));
        measurementNameLabel[channel]->setPalette(tablePalette);
        measurementMiscLabel.append(new QLabel());
        measurementMiscLabel[channel]->setPalette(tablePalette);
        measurementGainLabel.append(new QLabel());
        measurementGainLabel[channel]->setAlignment(Qt::AlignRight);
        measurementGainLabel[channel]->setPalette(tablePalette);
        tablePalette.setColor(QPalette::WindowText, settings->view.screen.spectrum[channel]);
        measurementMagnitudeLabel.append(new QLabel());
        measurementMagnitudeLabel[channel]->setAlignment(Qt::AlignRight);
        measurementMagnitudeLabel[channel]->setPalette(tablePalette);
        measurementAmplitudeLabel.append(new QLabel());
        measurementAmplitudeLabel[channel]->setAlignment(Qt::AlignRight);
        measurementAmplitudeLabel[channel]->setPalette(palette);
        measurementFrequencyLabel.append(new QLabel());
        measurementFrequencyLabel[channel]->setAlignment(Qt::AlignRight);
        measurementFrequencyLabel[channel]->setPalette(palette);
        setMeasurementVisible(channel, settings->scope.voltage[channel].used);
        measurementLayout->addWidget(measurementNameLabel[channel], channel, 0);
        measurementLayout->addWidget(measurementMiscLabel[channel], channel, 1);
        measurementLayout->addWidget(measurementGainLabel[channel], channel, 2);
        measurementLayout->addWidget(measurementMagnitudeLabel[channel], channel, 3);
        measurementLayout->addWidget(measurementAmplitudeLabel[channel], channel, 4);
        measurementLayout->addWidget(measurementFrequencyLabel[channel], channel, 5);
        if ((unsigned int)channel < settings->scope.physicalChannels)
            updateVoltageCoupling(channel);
        else
            updateMathMode();
        updateVoltageDetails(channel);
        updateSpectrumDetails(channel);
    }

    // The layout for the widgets
    mainLayout = new QGridLayout();
    mainLayout->setColumnStretch(2, 1); // Scopes increase their size
    mainLayout->setRowStretch(3, 1);
    // Bars around the scope, needed because the slider-drawing-area is outside
    // the scope at min/max
    mainLayout->setColumnMinimumWidth(1, triggerPositionSlider->preMargin());
    mainLayout->setColumnMinimumWidth(3, triggerPositionSlider->postMargin());
    mainLayout->setRowMinimumHeight(2, offsetSlider->preMargin());
    mainLayout->setRowMinimumHeight(4, offsetSlider->postMargin());
    mainLayout->setRowMinimumHeight(6, 4);
    mainLayout->setRowMinimumHeight(8, 4);
    mainLayout->setRowMinimumHeight(10, 8);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(settingsLayout, 0, 0, 1, 5);
    mainLayout->addWidget(mainScope, 3, 2);
    mainLayout->addWidget(offsetSlider, 2, 0, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(triggerPositionSlider, 1, 1, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(triggerLevelSlider, 2, 3, 3, 2, Qt::AlignLeft);
    mainLayout->addWidget(markerSlider, 4, 1, 2, 3, Qt::AlignTop);
    mainLayout->addLayout(markerLayout, 7, 0, 1, 5);
    mainLayout->addWidget(zoomScope, 9, 2);
    mainLayout->addLayout(measurementLayout, 11, 0, 1, 5);

    // Apply settings and update measured values
    updateTriggerDetails();
    updateRecordLength(settings->scope.horizontal.recordLength);
    updateFrequencybase(settings->scope.horizontal.frequencybase);
    updateSamplerate(settings->scope.horizontal.samplerate);
    updateTimebase(settings->scope.horizontal.timebase);
    updateZoom(settings->view.zoom);

    // The widget itself
    setPalette(palette);
    setBackgroundRole(QPalette::Background);
    setAutoFillBackground(true);
    setLayout(mainLayout);

    // Connect change-signals of sliders
    connect(offsetSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateOffset);
    connect(triggerPositionSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerPosition);
    connect(triggerLevelSlider, &LevelSlider::valueChanged, this, &DsoWidget::updateTriggerLevel);
    connect(markerSlider, &LevelSlider::valueChanged, [this](int index, double value) {
        updateMarker(index, value);
        mainScope->update();
        zoomScope->update();
    });
    updateTriggerSource();
}

void DsoWidget::showNewData(std::unique_ptr<DataAnalyzerResult> data) {
    if (!data) return;
    this->data = std::move(data);
    emit doShowNewData();
}

/// \brief Set the trigger level sliders minimum and maximum to the new values.
void DsoWidget::adaptTriggerLevelSlider(unsigned int channel) {
    triggerLevelSlider->setLimits(
        channel, (-DIVS_VOLTAGE / 2 - settings->scope.voltage[channel].offset) * settings->scope.voltage[channel].gain,
        (DIVS_VOLTAGE / 2 - settings->scope.voltage[channel].offset) * settings->scope.voltage[channel].gain);
    triggerLevelSlider->setStep(channel, settings->scope.voltage[channel].gain * 0.2);
}

/// \brief Show/Hide a line of the measurement table.
void DsoWidget::setMeasurementVisible(unsigned int channel, bool visible) {
    measurementNameLabel[channel]->setVisible(visible);
    measurementMiscLabel[channel]->setVisible(visible);
    measurementGainLabel[channel]->setVisible(visible);
    measurementMagnitudeLabel[channel]->setVisible(visible);
    measurementAmplitudeLabel[channel]->setVisible(visible);
    measurementFrequencyLabel[channel]->setVisible(visible);
    if (!visible) {
        measurementGainLabel[channel]->setText(QString());
        measurementMagnitudeLabel[channel]->setText(QString());
        measurementAmplitudeLabel[channel]->setText(QString());
        measurementFrequencyLabel[channel]->setText(QString());
    }
}

/// \brief Update the label about the marker measurements
void DsoWidget::updateMarkerDetails() {
    double divs = fabs(settings->scope.horizontal.marker[1] - settings->scope.horizontal.marker[0]);
    double time = divs * settings->scope.horizontal.timebase;

    if (settings->view.zoom) {
        markerInfoLabel->setText(tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));
        markerTimebaseLabel->setText(valueToString(time / DIVS_TIME, UNIT_SECONDS, 3) + tr("/div"));
        markerFrequencybaseLabel->setText(
            valueToString(divs * settings->scope.horizontal.frequencybase / DIVS_TIME, UNIT_HERTZ, 4) + tr("/div"));
    }
    markerTimeLabel->setText(valueToString(time, UNIT_SECONDS, 4));
    markerFrequencyLabel->setText(valueToString(1.0 / time, UNIT_HERTZ, 4));
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateSpectrumDetails(unsigned int channel) {
    setMeasurementVisible(channel, settings->scope.voltage[channel].used || settings->scope.spectrum[channel].used);

    if (settings->scope.spectrum[channel].used)
        measurementMagnitudeLabel[channel]->setText(
            valueToString(settings->scope.spectrum[channel].magnitude, UNIT_DECIBEL, 3) + tr("/div"));
    else
        measurementMagnitudeLabel[channel]->setText(QString());
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateTriggerDetails() {
    // Update the trigger details
    QPalette tablePalette = palette();
    tablePalette.setColor(QPalette::WindowText, settings->view.screen.voltage[settings->scope.trigger.source]);
    settingsTriggerLabel->setPalette(tablePalette);
    QString levelString = valueToString(settings->scope.voltage[settings->scope.trigger.source].trigger, UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int)(settings->scope.trigger.position * 100 + 0.5));
    settingsTriggerLabel->setText(tr("%1  %2  %3  %4")
                                      .arg(settings->scope.voltage[settings->scope.trigger.source].name,
                                           Dso::slopeString(settings->scope.trigger.slope), levelString,
                                           pretriggerString));

    /// \todo This won't work for special trigger sources
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateVoltageDetails(unsigned int channel) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    setMeasurementVisible(channel, settings->scope.voltage[channel].used || settings->scope.spectrum[channel].used);

    if (settings->scope.voltage[channel].used)
        measurementGainLabel[channel]->setText(valueToString(settings->scope.voltage[channel].gain, UNIT_VOLTS, 3) +
                                               tr("/div"));
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
void DsoWidget::updateSpectrumMagnitude(unsigned int channel) { updateSpectrumDetails(channel); }

/// \brief Handles usedChanged signal from the spectrum dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateSpectrumUsed(unsigned int channel, bool used) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    offsetSlider->setVisible(settings->scope.voltage.count() + channel, used);

    updateSpectrumDetails(channel);
}

/// \brief Handles modeChanged signal from the trigger dock.
void DsoWidget::updateTriggerMode() { updateTriggerDetails(); }

/// \brief Handles slopeChanged signal from the trigger dock.
void DsoWidget::updateTriggerSlope() { updateTriggerDetails(); }

/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
    // Change the colors of the trigger sliders
    if (settings->scope.trigger.special || settings->scope.trigger.source >= settings->scope.physicalChannels)
        triggerPositionSlider->setColor(0, settings->view.screen.border);
    else
        triggerPositionSlider->setColor(0, settings->view.screen.voltage[settings->scope.trigger.source]);

    for (int channel = 0; channel < (int)settings->scope.physicalChannels; ++channel)
        triggerLevelSlider->setColor(
            channel,
            (!settings->scope.trigger.special && channel == (int)settings->scope.trigger.source)
                ? settings->view.screen.voltage[channel]
                : settings->view.screen.voltage[channel].darker());

    updateTriggerDetails();
}

/// \brief Handles couplingChanged signal from the voltage dock.
/// \param channel The channel whose coupling was changed.
void DsoWidget::updateVoltageCoupling(unsigned int channel) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    measurementMiscLabel[channel]->setText(Dso::couplingString(settings->scope.voltage[channel].coupling));
}

/// \brief Handles modeChanged signal from the voltage dock.
void DsoWidget::updateMathMode() {
    measurementMiscLabel[settings->scope.physicalChannels]->setText(
        Dso::mathModeString(settings->scope.voltage[settings->scope.physicalChannels].math));
}

/// \brief Handles gainChanged signal from the voltage dock.
/// \param channel The channel whose gain was changed.
void DsoWidget::updateVoltageGain(unsigned int channel) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    if (channel < settings->scope.physicalChannels) adaptTriggerLevelSlider(channel);

    updateVoltageDetails(channel);
}

/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateVoltageUsed(unsigned int channel, bool used) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    offsetSlider->setVisible(channel, used);
    triggerLevelSlider->setVisible(channel, used);
    setMeasurementVisible(channel, settings->scope.voltage[channel].used);

    updateVoltageDetails(channel);
}

/// \brief Change the record length.
void DsoWidget::updateRecordLength(unsigned long size) {
    settingsRecordLengthLabel->setText(valueToString(size, UNIT_SAMPLES, 4));
}

/// \brief Export the oscilloscope screen to a file.
/// \return true if the document was exported successfully.
void DsoWidget::exportAs() { exportNextFrame.reset(Exporter::createSaveToFileExporter(settings)); }

/// \brief Print the oscilloscope screen.
/// \return true if the document was sent to the printer successfully.
void DsoWidget::print() { exportNextFrame.reset(Exporter::createPrintExporter(settings)); }

/// \brief Stop the oscilloscope.
void DsoWidget::updateZoom(bool enabled) {
    mainLayout->setRowStretch(9, enabled ? 1 : 0);
    zoomScope->setVisible(enabled);

    // Show time-/frequencybase and zoom factor if the magnified scope is shown
    markerLayout->setStretch(3, enabled ? 1 : 0);
    markerTimebaseLabel->setVisible(enabled);
    markerLayout->setStretch(4, enabled ? 1 : 0);
    markerFrequencybaseLabel->setVisible(enabled);
    if (enabled)
        updateMarkerDetails();
    else
        markerInfoLabel->setText(tr("Marker 1/2"));

    repaint();
}

/// \brief Prints analyzed data.
void DsoWidget::doShowNewData() {
    if (exportNextFrame) {
        exportNextFrame->exportSamples(data.get());
        exportNextFrame.reset(nullptr);
    }

    generator->generateGraphs(data.get());

    updateRecordLength(data.get()->getMaxSamples());

    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        if (settings->scope.voltage[channel].used && data.get()->data(channel)) {
            // Amplitude string representation (4 significant digits)
            measurementAmplitudeLabel[channel]->setText(
                valueToString(data.get()->data(channel)->amplitude, UNIT_VOLTS, 4));
            // Frequency string representation (5 significant digits)
            measurementFrequencyLabel[channel]->setText(
                valueToString(data.get()->data(channel)->frequency, UNIT_HERTZ, 5));
        }
    }
}

/// \brief Handles valueChanged signal from the offset sliders.
/// \param channel The channel whose offset was changed.
/// \param value The new offset for the channel.
void DsoWidget::updateOffset(int channel, double value) {
    if (channel < settings->scope.voltage.count()) {
        settings->scope.voltage[channel].offset = value;

        if (channel < (int)settings->scope.physicalChannels) adaptTriggerLevelSlider(channel);
    } else if (channel < settings->scope.voltage.count() * 2)
        settings->scope.spectrum[channel - settings->scope.voltage.count()].offset = value;

    emit offsetChanged(channel, value);
}

/// \brief Handles valueChanged signal from the triggerPosition slider.
/// \param index The index of the slider.
/// \param value The new triggerPosition in seconds relative to the first
/// sample.
void DsoWidget::updateTriggerPosition(int index, double value) {
    if (index != 0) return;

    settings->scope.trigger.position = value;

    updateTriggerDetails();

    emit triggerPositionChanged(value * settings->scope.horizontal.timebase * DIVS_TIME);
}

/// \brief Handles valueChanged signal from the trigger level slider.
/// \param channel The index of the slider.
/// \param value The new trigger level.
void DsoWidget::updateTriggerLevel(int channel, double value) {
    settings->scope.voltage[channel].trigger = value;

    updateTriggerDetails();

    emit triggerLevelChanged(channel, value);
}

/// \brief Handles valueChanged signal from the marker slider.
/// \param marker The index of the slider.
/// \param value The new marker position.
void DsoWidget::updateMarker(int marker, double value) {
    settings->scope.horizontal.marker[marker] = value;

    updateMarkerDetails();

    emit markerChanged(marker, value);
}
