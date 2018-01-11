// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QPainter>
#include <QPrinter>
#include <QSize>
#include <memory>
#include "exportsettings.h"

class DsoSettings;
class PPresult;
struct DsoSettingsColorValues;
namespace Dso { struct ControlSpecification; }

/// \brief Exports the oscilloscope screen to a file or prints it.
class Exporter {
  public:
    static Exporter *createPrintExporter(const Dso::ControlSpecification* deviceSpecification, DsoSettings *settings);
    static Exporter *createSaveToFileExporter(const Dso::ControlSpecification* deviceSpecification, DsoSettings *settings);

    /// \brief Print the document (May be a file too)
    bool exportSamples(const PPresult *result);

  private:
    Exporter(const Dso::ControlSpecification* deviceSpecification, DsoSettings *settings, const QString &filename, ExportFormat format);
    void setFormat(ExportFormat format);
    bool exportCSV(const PPresult *result);
    static std::unique_ptr<QPrinter> printPaintDevice(DsoSettings *settings);
    void drawGrids(QPainter &painter, DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                   int scopeWidth);
    const Dso::ControlSpecification* deviceSpecification;
    DsoSettings *settings;
    std::unique_ptr<QPrinter> selectedPrinter;

    QString filename;
    ExportFormat format;
};
