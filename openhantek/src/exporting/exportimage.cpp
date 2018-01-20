// SPDX-License-Identifier: GPL-2.0+

#include "exportimage.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "legacyexportdrawer.h"
#include "post/ppresult.h"
#include "settings.h"
#include "viewsettings.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QPrinter>

ExporterImage::ExporterImage() {}

void ExporterImage::create(ExporterRegistry *registry) { this->registry = registry; data.reset(); }

QIcon ExporterImage::icon() { return iconFont->icon(fa::image); }

QString ExporterImage::name() { return QCoreApplication::tr("Export Image/PDF"); }

ExporterInterface::Type ExporterImage::type() { return Type::SnapshotExport; }

bool ExporterImage::samples(const std::shared_ptr<PPresult> data) {
    this->data = std::move(data);
    return false;
}

bool ExporterImage::save() {
    QStringList filters;
    filters << QCoreApplication::tr("Portable Document Format (*.pdf)")
            << QCoreApplication::tr("Image (*.png *.xpm *.jpg)");

    QFileDialog fileDialog(nullptr, QCoreApplication::tr("Export file..."), QString(), filters.join(";;"));
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    if (fileDialog.exec() != QDialog::Accepted) return false;

    bool isPdf = filters.indexOf(fileDialog.selectedNameFilter()) == 0;
    std::unique_ptr<QPaintDevice> paintDevice;

    const DsoSettingsColorValues *colorValues = &(registry->settings->view.print);

    if (!isPdf) {
        // We need a QPixmap for image-export
        QPixmap *qPixmap = new QPixmap(registry->settings->exporting.imageSize);
        qPixmap->fill(colorValues->background);
        paintDevice = std::unique_ptr<QPaintDevice>(qPixmap);
    } else {
        // We need a QPrinter for printing, pdf- and ps-export
        std::unique_ptr<QPrinter> printer = std::unique_ptr<QPrinter>(new QPrinter(QPrinter::HighResolution));
        printer->setOrientation(registry->settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape);
        printer->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);
        printer->setOutputFileName(fileDialog.selectedFiles().first());
        printer->setOutputFormat(QPrinter::PdfFormat);
        paintDevice = std::move(printer);
    }

    if (!paintDevice) return false;

    LegacyExportDrawer::exportSamples(data.get(), paintDevice.get(), registry->deviceSpecification, registry->settings,
                                      false, colorValues);

    if (!isPdf) static_cast<QPixmap *>(paintDevice.get())->save(fileDialog.selectedFiles().first());
    return true;
}

float ExporterImage::progress() { return data ? 1.0f : 0; }
