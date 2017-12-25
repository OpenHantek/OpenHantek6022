////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  settings.cpp
//
//  Copyright (C) 2010, 2011  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "settings.h"

#include "dsowidget.h"

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
DsoSettings::DsoSettings(unsigned int channels) {
    scope.physicalChannels = channels;

    DsoSettingsScopeSpectrum newSpectrum;
    newSpectrum.name = QApplication::tr("SPM");
    scope.spectrum.push_back(newSpectrum);

    DsoSettingsScopeVoltage newVoltage;
    newVoltage.math = Dso::MathMode::ADD_CH1_CH2;
    newVoltage.name = QApplication::tr("MATH");
    scope.voltage.push_back(newVoltage);

    view.screen.voltage.push_back(QColor(0x7f, 0x7f, 0x7f, 0xff));
    view.screen.spectrum.push_back(view.screen.voltage.back().lighter());
    view.print.voltage.push_back(view.screen.voltage.back());
    view.print.spectrum.push_back(view.print.voltage.back().darker());

    // Add new channels to the list
    while (scope.spectrum.size() <= channels) {
        // Spectrum
        DsoSettingsScopeSpectrum newSpectrum;
        newSpectrum.name = QApplication::tr("SP%1").arg(scope.spectrum.size());
        scope.spectrum.insert(scope.spectrum.end()-1, newSpectrum);

        // Voltage
        DsoSettingsScopeVoltage newVoltage;
        newVoltage.coupling = Dso::COUPLING_DC;
        newVoltage.name = QApplication::tr("CH%1").arg(scope.voltage.size());
        scope.voltage.insert(scope.voltage.end()-1, newVoltage);

        view.screen.voltage.insert(view.screen.voltage.end()-1, QColor::fromHsv((int)(view.screen.voltage.size()-1) * 60, 0xff, 0xff));
        view.screen.spectrum.insert(view.screen.spectrum.end()-1, view.screen.voltage.back().lighter());
        view.print.voltage.insert(view.print.voltage.end()-1, view.screen.voltage.back().darker(120));
        view.print.spectrum.insert(view.print.spectrum.end()-1, view.screen.voltage.back().darker());
    }

    load();
}

bool DsoSettings::setFilename(const QString &filename) {
    std::unique_ptr<QSettings> local = std::unique_ptr<QSettings>(new QSettings(filename, QSettings::IniFormat));
    if (local->status() != QSettings::NoError) {
        qWarning() << "Could not change the settings file to " << filename;
        return false;
    }
    store.swap(local);
    return true;
}

void DsoSettings::load() {
    // General options
    store->beginGroup("options");
    if (store->contains("alwaysSave")) options.alwaysSave = store->value("alwaysSave").toBool();
    if (store->contains("imageSize")) options.imageSize = store->value("imageSize").toSize();
    // If the window/* keys were found in this group, remove them from settings
    store->remove("window");
    store->endGroup();

    // Oscilloscope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    if (store->contains("format")) scope.horizontal.format = (Dso::GraphFormat)store->value("format").toInt();
    if (store->contains("frequencybase"))
        scope.horizontal.frequencybase = store->value("frequencybase").toDouble();
    for (int marker = 0; marker < 2; ++marker) {
        QString name;
        name = QString("marker%1").arg(marker);
        if (store->contains(name)) scope.horizontal.marker[marker] = store->value(name).toDouble();
    }
    if (store->contains("timebase")) scope.horizontal.timebase = store->value("timebase").toDouble();
    if (store->contains("recordLength")) scope.horizontal.recordLength = store->value("recordLength").toUInt();
    if (store->contains("samplerate")) scope.horizontal.samplerate = store->value("samplerate").toDouble();
    if (store->contains("samplerateSet")) scope.horizontal.samplerateSet = store->value("samplerateSet").toBool();
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    if (store->contains("filter")) scope.trigger.filter = store->value("filter").toBool();
    if (store->contains("mode")) scope.trigger.mode = (Dso::TriggerMode)store->value("mode").toInt();
    if (store->contains("position")) scope.trigger.position = store->value("position").toDouble();
    if (store->contains("slope")) scope.trigger.slope = (Dso::Slope)store->value("slope").toInt();
    if (store->contains("source")) scope.trigger.source = store->value("source").toInt();
    if (store->contains("special")) scope.trigger.special = store->value("special").toInt();
    store->endGroup();
    // Spectrum
    for (unsigned channel = 0; channel < scope.spectrum.size(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        if (store->contains("magnitude"))
            scope.spectrum[channel].magnitude = store->value("magnitude").toDouble();
        if (store->contains("offset")) scope.spectrum[channel].offset = store->value("offset").toDouble();
        if (store->contains("used")) scope.spectrum[channel].used = store->value("used").toBool();
        store->endGroup();
    }
    // Vertical axis
    for (unsigned channel = 0; channel < scope.voltage.size(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        if (store->contains("gain")) scope.voltage[channel].gain = store->value("gain").toDouble();
        if (store->contains("inverted")) scope.voltage[channel].inverted = store->value("inverted").toBool();
        if (store->contains("misc")) scope.voltage[channel].rawValue = store->value("misc").toInt();
        if (store->contains("offset")) scope.voltage[channel].offset = store->value("offset").toDouble();
        if (store->contains("trigger")) scope.voltage[channel].trigger = store->value("trigger").toDouble();
        if (store->contains("used")) scope.voltage[channel].used = store->value("used").toBool();
        store->endGroup();
    }
    if (store->contains("spectrumLimit")) scope.spectrumLimit = store->value("spectrumLimit").toDouble();
    if (store->contains("spectrumReference"))
        scope.spectrumReference = store->value("spectrumReference").toDouble();
    if (store->contains("spectrumWindow"))
        scope.spectrumWindow = (Dso::WindowFunction)store->value("spectrumWindow").toInt();
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors
    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &view.screen;
            store->beginGroup("screen");
        } else {
            colors = &view.print;
            store->beginGroup("print");
        }

        if (store->contains("axes")) colors->axes = store->value("axes").value<QColor>();
        if (store->contains("background")) colors->background = store->value("background").value<QColor>();
        if (store->contains("border")) colors->border = store->value("border").value<QColor>();
        if (store->contains("grid")) colors->grid = store->value("grid").value<QColor>();
        if (store->contains("markers")) colors->markers = store->value("markers").value<QColor>();
        for (unsigned channel = 0; channel < scope.spectrum.size(); ++channel) {
            QString key = QString("spectrum%1").arg(channel);
            if (store->contains(key)) colors->spectrum[channel] = store->value(key).value<QColor>();
        }
        if (store->contains("text")) colors->text = store->value("text").value<QColor>();
        for (unsigned channel = 0; channel < scope.voltage.size(); ++channel) {
            QString key = QString("voltage%1").arg(channel);
            if (store->contains(key)) colors->voltage[channel] = store->value(key).value<QColor>();
        }
        store->endGroup();
    }
    store->endGroup();
    // Other view settings
    if (store->contains("digitalPhosphor")) view.digitalPhosphor = store->value("digitalPhosphor").toBool();
    if (store->contains("interpolation"))
        view.interpolation = (Dso::InterpolationMode)store->value("interpolation").toInt();
    if (store->contains("screenColorImages")) view.screenColorImages = store->value("screenColorImages").toBool();
    if (store->contains("zoom")) view.zoom = (Dso::InterpolationMode)store->value("zoom").toBool();
    store->endGroup();

    store->beginGroup("window");
    mainWindowGeometry = store->value("geometry").toByteArray();
    mainWindowState = store->value("state").toByteArray();
    store->endGroup();
}

void DsoSettings::save() {
    // Main window layout and other general options
    store->beginGroup("options");
    store->setValue("alwaysSave", options.alwaysSave);
    store->setValue("imageSize", options.imageSize);
    store->endGroup();

    // Oszilloskope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    store->setValue("format", scope.horizontal.format);
    store->setValue("frequencybase", scope.horizontal.frequencybase);
    for (int marker = 0; marker < 2; ++marker)
        store->setValue(QString("marker%1").arg(marker), scope.horizontal.marker[marker]);
    store->setValue("timebase", scope.horizontal.timebase);
    store->setValue("recordLength", scope.horizontal.recordLength);
    store->setValue("samplerate", scope.horizontal.samplerate);
    store->setValue("samplerateSet", scope.horizontal.samplerateSet);
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    store->setValue("filter", scope.trigger.filter);
    store->setValue("mode", scope.trigger.mode);
    store->setValue("position", scope.trigger.position);
    store->setValue("slope", scope.trigger.slope);
    store->setValue("source", scope.trigger.source);
    store->setValue("special", scope.trigger.special);
    store->endGroup();
    // Spectrum
    for (unsigned channel = 0; channel < scope.spectrum.size(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        store->setValue("magnitude", scope.spectrum[channel].magnitude);
        store->setValue("offset", scope.spectrum[channel].offset);
        store->setValue("used", scope.spectrum[channel].used);
        store->endGroup();
    }
    // Vertical axis
    for (unsigned channel = 0; channel < scope.voltage.size(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        store->setValue("gain", scope.voltage[channel].gain);
        store->setValue("inverted", scope.voltage[channel].inverted);
        store->setValue("misc", scope.voltage[channel].rawValue);
        store->setValue("offset", scope.voltage[channel].offset);
        store->setValue("trigger", scope.voltage[channel].trigger);
        store->setValue("used", scope.voltage[channel].used);
        store->endGroup();
    }
    store->setValue("spectrumLimit", scope.spectrumLimit);
    store->setValue("spectrumReference", scope.spectrumReference);
    store->setValue("spectrumWindow", (int)scope.spectrumWindow);
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors

    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &view.screen;
            store->beginGroup("screen");
        } else {
            colors = &view.print;
            store->beginGroup("print");
        }

        store->setValue("axes", colors->axes.name(QColor::HexArgb));
        store->setValue("background", colors->background.name(QColor::HexArgb));
        store->setValue("border", colors->border.name(QColor::HexArgb));
        store->setValue("grid", colors->grid.name(QColor::HexArgb));
        store->setValue("markers", colors->markers.name(QColor::HexArgb));
        for (unsigned channel = 0; channel < scope.spectrum.size(); ++channel)
            store->setValue(QString("spectrum%1").arg(channel), colors->spectrum[channel].name(QColor::HexArgb));
        store->setValue("text", colors->text.name(QColor::HexArgb));
        for (unsigned channel = 0; channel < scope.voltage.size(); ++channel)
            store->setValue(QString("voltage%1").arg(channel), colors->voltage[channel].name(QColor::HexArgb));
        store->endGroup();
    }
    store->endGroup();

    // Other view settings
    store->setValue("digitalPhosphor", view.digitalPhosphor);
    store->setValue("interpolation", view.interpolation);
    store->setValue("screenColorImages", view.screenColorImages);
    store->setValue("zoom", view.zoom);
    store->endGroup();

    store->beginGroup("window");
    store->setValue("geometry", mainWindowGeometry);
    store->setValue("state", mainWindowState);
    store->endGroup();
}
