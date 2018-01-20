// SPDX-License-Identifier: GPL-2.0+

#include "exportprint.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "legacyexportdrawer.h"
#include "post/ppresult.h"
#include "settings.h"
#include "viewsettings.h"

#include <QCoreApplication>
#include <QPrintDialog>
#include <QPrinter>

ExporterPrint::ExporterPrint() {}

void ExporterPrint::create(ExporterRegistry *registry) { this->registry = registry; data.reset(); }

QIcon ExporterPrint::icon() { return iconFont->icon(fa::print); }

QString ExporterPrint::name() { return QCoreApplication::tr("Print"); }

ExporterInterface::Type ExporterPrint::type() { return Type::SnapshotExport; }

bool ExporterPrint::samples(const std::shared_ptr<PPresult> data) {
    this->data = std::move(data);
    return false;
}

bool ExporterPrint::save() {
    // We need a QPrinter for printing, pdf- and ps-export
    std::unique_ptr<QPrinter> printer = std::unique_ptr<QPrinter>(new QPrinter(QPrinter::HighResolution));
    printer->setOrientation(registry->settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape);
    printer->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);

    // Show the printing dialog
    QPrintDialog dialog(printer.get());
    dialog.setWindowTitle(QCoreApplication::tr("Print oscillograph"));
    if (dialog.exec() != QDialog::Accepted) { return false; }

    const DsoSettingsColorValues *colorValues = &(registry->settings->view.print);

    LegacyExportDrawer::exportSamples(data.get(), printer.get(), registry->deviceSpecification, registry->settings,
                                      true, colorValues);

    return true;
}

float ExporterPrint::progress() { return data ? 1.0f : 0; }
