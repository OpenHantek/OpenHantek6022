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

#include "definitions.h"
#include "dsowidget.h"

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
DsoSettings::DsoSettings() { load(); }

bool DsoSettings::setFilename(const QString &filename) {
    std::unique_ptr<QSettings> local = std::unique_ptr<QSettings>(new QSettings(filename, QSettings::IniFormat));
    if (local->status() != QSettings::NoError) {
        qWarning() << "Could not change the settings file to " << filename;
        return false;
    }
    store.swap(local);
    return true;
}

void DsoSettings::setChannelCount(unsigned int channels) {
    this->scope.physicalChannels = channels;
    // Always put the math channel at the end of the list

    // Remove list items for removed channels
    for (int channel = this->scope.spectrum.count() - 2; channel >= (int)channels; channel--)
        this->scope.spectrum.removeAt(channel);
    for (int channel = this->scope.voltage.count() - 2; channel >= (int)channels; channel--)
        this->scope.voltage.removeAt(channel);
    for (int channel = this->scope.spectrum.count() - 2; channel >= (int)channels; channel--)
        this->scope.spectrum.removeAt(channel);
    for (int channel = this->scope.spectrum.count() - 2; channel >= (int)channels; channel--)
        this->scope.spectrum.removeAt(channel);

    // Add new channels to the list
    for (int channel = 0; channel < (int)channels; ++channel) {
        // Oscilloscope settings
        // Spectrum
        if (this->scope.spectrum.count() <= channel + 1) {
            DsoSettingsScopeSpectrum newSpectrum;
            newSpectrum.magnitude = 20.0;
            newSpectrum.name = QApplication::tr("SP%1").arg(channel + 1);
            newSpectrum.offset = 0.0;
            newSpectrum.used = false;
            this->scope.spectrum.insert(channel, newSpectrum);
        }
        // Voltage
        if (this->scope.voltage.count() <= channel + 1) {
            DsoSettingsScopeVoltage newVoltage;
            newVoltage.gain = 1.0;
            newVoltage.misc = Dso::COUPLING_DC;
            newVoltage.name = QApplication::tr("CH%1").arg(channel + 1);
            newVoltage.offset = 0.0;
            newVoltage.trigger = 0.0;
            newVoltage.used = false;
            this->scope.voltage.insert(channel, newVoltage);
        }

        // View
        // Colors
        // Screen
        if (this->view.screen.voltage.count() <= channel + 1)
            this->view.screen.voltage.insert(channel, QColor::fromHsv(channel * 60, 0xff, 0xff));
        if (this->view.screen.spectrum.count() <= channel + 1)
            this->view.screen.spectrum.insert(channel, this->view.screen.voltage[channel].lighter());
        // Print
        if (this->view.print.voltage.count() <= channel + 1)
            this->view.print.voltage.insert(channel, this->view.screen.voltage[channel].darker(120));
        if (this->view.print.spectrum.count() <= channel + 1)
            this->view.print.spectrum.insert(channel, this->view.screen.voltage[channel].darker());
    }

    // Check if the math channel is missing
    if (this->scope.spectrum.count() <= (int)channels) {
        DsoSettingsScopeSpectrum newSpectrum;
        newSpectrum.magnitude = 20.0;
        newSpectrum.name = QApplication::tr("SPM");
        newSpectrum.offset = 0.0;
        newSpectrum.used = false;
        this->scope.spectrum.append(newSpectrum);
    }
    if (this->scope.voltage.count() <= (int)channels) {
        DsoSettingsScopeVoltage newVoltage;
        newVoltage.gain = 1.0;
        newVoltage.misc = Dso::MATHMODE_1ADD2;
        newVoltage.name = QApplication::tr("MATH");
        newVoltage.offset = 0.0;
        newVoltage.trigger = 0.0;
        newVoltage.used = false;
        this->scope.voltage.append(newVoltage);
    }
    if (this->view.screen.voltage.count() <= (int)channels)
        this->view.screen.voltage.append(QColor(0x7f, 0x7f, 0x7f, 0xff));
    if (this->view.screen.spectrum.count() <= (int)channels)
        this->view.screen.spectrum.append(this->view.screen.voltage[channels].lighter());
    if (this->view.print.voltage.count() <= (int)channels)
        this->view.print.voltage.append(this->view.screen.voltage[channels]);
    if (this->view.print.spectrum.count() <= (int)channels)
        this->view.print.spectrum.append(this->view.print.voltage[channels].darker());
}

void DsoSettings::load() {
    // General options
    store->beginGroup("options");
    if (store->contains("alwaysSave")) this->options.alwaysSave = store->value("alwaysSave").toBool();
    if (store->contains("imageSize")) this->options.imageSize = store->value("imageSize").toSize();
    // If the window/* keys were found in this group, remove them from settings
    store->remove("window");
    store->endGroup();

    // Oscilloscope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    if (store->contains("format")) this->scope.horizontal.format = (Dso::GraphFormat)store->value("format").toInt();
    if (store->contains("frequencybase"))
        this->scope.horizontal.frequencybase = store->value("frequencybase").toDouble();
    for (int marker = 0; marker < 2; ++marker) {
        QString name;
        name = QString("marker%1").arg(marker);
        if (store->contains(name)) this->scope.horizontal.marker[marker] = store->value(name).toDouble();
    }
    if (store->contains("timebase")) this->scope.horizontal.timebase = store->value("timebase").toDouble();
    if (store->contains("recordLength")) this->scope.horizontal.recordLength = store->value("recordLength").toUInt();
    if (store->contains("samplerate")) this->scope.horizontal.samplerate = store->value("samplerate").toDouble();
    if (store->contains("samplerateSet")) this->scope.horizontal.samplerateSet = store->value("samplerateSet").toBool();
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    if (store->contains("filter")) this->scope.trigger.filter = store->value("filter").toBool();
    if (store->contains("mode")) this->scope.trigger.mode = (Dso::TriggerMode)store->value("mode").toInt();
    if (store->contains("position")) this->scope.trigger.position = store->value("position").toDouble();
    if (store->contains("slope")) this->scope.trigger.slope = (Dso::Slope)store->value("slope").toInt();
    if (store->contains("source")) this->scope.trigger.source = store->value("source").toInt();
    if (store->contains("special")) this->scope.trigger.special = store->value("special").toInt();
    store->endGroup();
    // Spectrum
    for (int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        if (store->contains("magnitude"))
            this->scope.spectrum[channel].magnitude = store->value("magnitude").toDouble();
        if (store->contains("offset")) this->scope.spectrum[channel].offset = store->value("offset").toDouble();
        if (store->contains("used")) this->scope.spectrum[channel].used = store->value("used").toBool();
        store->endGroup();
    }
    // Vertical axis
    for (int channel = 0; channel < this->scope.voltage.count(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        if (store->contains("gain")) this->scope.voltage[channel].gain = store->value("gain").toDouble();
        if (store->contains("misc")) this->scope.voltage[channel].misc = store->value("misc").toInt();
        if (store->contains("offset")) this->scope.voltage[channel].offset = store->value("offset").toDouble();
        if (store->contains("trigger")) this->scope.voltage[channel].trigger = store->value("trigger").toDouble();
        if (store->contains("used")) this->scope.voltage[channel].used = store->value("used").toBool();
        store->endGroup();
    }
    if (store->contains("spectrumLimit")) this->scope.spectrumLimit = store->value("spectrumLimit").toDouble();
    if (store->contains("spectrumReference"))
        this->scope.spectrumReference = store->value("spectrumReference").toDouble();
    if (store->contains("spectrumWindow"))
        this->scope.spectrumWindow = (Dso::WindowFunction)store->value("spectrumWindow").toInt();
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors
    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &this->view.screen;
            store->beginGroup("screen");
        } else {
            colors = &this->view.print;
            store->beginGroup("print");
        }

        if (store->contains("axes")) colors->axes = store->value("axes").value<QColor>();
        if (store->contains("background")) colors->background = store->value("background").value<QColor>();
        if (store->contains("border")) colors->border = store->value("border").value<QColor>();
        if (store->contains("grid")) colors->grid = store->value("grid").value<QColor>();
        if (store->contains("markers")) colors->markers = store->value("markers").value<QColor>();
        for (int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
            QString key = QString("spectrum%1").arg(channel);
            if (store->contains(key)) colors->spectrum[channel] = store->value(key).value<QColor>();
        }
        if (store->contains("text")) colors->text = store->value("text").value<QColor>();
        for (int channel = 0; channel < this->scope.voltage.count(); ++channel) {
            QString key = QString("voltage%1").arg(channel);
            if (store->contains(key)) colors->voltage[channel] = store->value(key).value<QColor>();
        }
        store->endGroup();
    }
    store->endGroup();
    // Other view settings
    if (store->contains("digitalPhosphor")) this->view.digitalPhosphor = store->value("digitalPhosphor").toBool();
    if (store->contains("interpolation"))
        this->view.interpolation = (Dso::InterpolationMode)store->value("interpolation").toInt();
    if (store->contains("screenColorImages")) this->view.screenColorImages = store->value("screenColorImages").toBool();
    if (store->contains("zoom")) this->view.zoom = (Dso::InterpolationMode)store->value("zoom").toBool();
    store->endGroup();

    store->beginGroup("window");
    mainWindowGeometry = store->value("geometry").toByteArray();
    mainWindowState = store->value("state").toByteArray();
    store->endGroup();
}

void DsoSettings::save() {
    // Main window layout and other general options
    store->beginGroup("options");
    store->setValue("alwaysSave", this->options.alwaysSave);
    store->setValue("imageSize", this->options.imageSize);
    store->endGroup();

    // Oszilloskope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    store->setValue("format", this->scope.horizontal.format);
    store->setValue("frequencybase", this->scope.horizontal.frequencybase);
    for (int marker = 0; marker < 2; ++marker)
        store->setValue(QString("marker%1").arg(marker), this->scope.horizontal.marker[marker]);
    store->setValue("timebase", this->scope.horizontal.timebase);
    store->setValue("recordLength", this->scope.horizontal.recordLength);
    store->setValue("samplerate", this->scope.horizontal.samplerate);
    store->setValue("samplerateSet", this->scope.horizontal.samplerateSet);
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    store->setValue("filter", this->scope.trigger.filter);
    store->setValue("mode", this->scope.trigger.mode);
    store->setValue("position", this->scope.trigger.position);
    store->setValue("slope", this->scope.trigger.slope);
    store->setValue("source", this->scope.trigger.source);
    store->setValue("special", this->scope.trigger.special);
    store->endGroup();
    // Spectrum
    for (int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        store->setValue("magnitude", this->scope.spectrum[channel].magnitude);
        store->setValue("offset", this->scope.spectrum[channel].offset);
        store->setValue("used", this->scope.spectrum[channel].used);
        store->endGroup();
    }
    // Vertical axis
    for (int channel = 0; channel < this->scope.voltage.count(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        store->setValue("gain", this->scope.voltage[channel].gain);
        store->setValue("misc", this->scope.voltage[channel].misc);
        store->setValue("offset", this->scope.voltage[channel].offset);
        store->setValue("trigger", this->scope.voltage[channel].trigger);
        store->setValue("used", this->scope.voltage[channel].used);
        store->endGroup();
    }
    store->setValue("spectrumLimit", this->scope.spectrumLimit);
    store->setValue("spectrumReference", this->scope.spectrumReference);
    store->setValue("spectrumWindow", this->scope.spectrumWindow);
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors

    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &this->view.screen;
            store->beginGroup("screen");
        } else {
            colors = &this->view.print;
            store->beginGroup("print");
        }

        store->setValue("axes", colors->axes);
        store->setValue("background", colors->background);
        store->setValue("border", colors->border);
        store->setValue("grid", colors->grid);
        store->setValue("markers", colors->markers);
        for (int channel = 0; channel < this->scope.spectrum.count(); ++channel)
            store->setValue(QString("spectrum%1").arg(channel), colors->spectrum[channel]);
        store->setValue("text", colors->text);
        for (int channel = 0; channel < this->scope.voltage.count(); ++channel)
            store->setValue(QString("voltage%1").arg(channel), colors->voltage[channel]);
        store->endGroup();
    }
    store->endGroup();

    // Other view settings
    store->setValue("digitalPhosphor", this->view.digitalPhosphor);
    store->setValue("interpolation", this->view.interpolation);
    store->setValue("screenColorImages", this->view.screenColorImages);
    store->setValue("zoom", this->view.zoom);
    store->endGroup();

    store->beginGroup("window");
    store->setValue("geometry", mainWindowGeometry);
    store->setValue("state", mainWindowState);
    store->endGroup();
}
