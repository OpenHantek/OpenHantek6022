// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QPainter>
#include <QPrinter>
#include <QSize>
#include <memory>

class DsoSettings;
class DataAnalyzerResult;
struct DsoSettingsColorValues;

////////////////////////////////////////////////////////////////////////////////
/// \enum ExportFormat                                                exporter.h
/// \brief Possible file formats for the export.
enum ExportFormat { EXPORT_FORMAT_PRINTER, EXPORT_FORMAT_PDF, EXPORT_FORMAT_IMAGE, EXPORT_FORMAT_CSV };

////////////////////////////////////////////////////////////////////////////////
/// \class Exporter                                                   exporter.h
/// \brief Exports the oscilloscope screen to a file or prints it.
class Exporter {
  public:
    static Exporter *createPrintExporter(DsoSettings *settings);
    static Exporter *createSaveToFileExporter(DsoSettings *settings);

    /// \brief Print the document (May be a file too)
    bool exportSamples(const DataAnalyzerResult *result);

  private:
    Exporter(DsoSettings *settings, const QString &filename, ExportFormat format);
    void setFormat(ExportFormat format);
    bool exportCVS(const DataAnalyzerResult *result);
    static std::unique_ptr<QPrinter> printPaintDevice(DsoSettings *settings);
    void drawGrids(QPainter &painter, DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                   int scopeWidth);
    DsoSettings *settings;
    std::unique_ptr<QPrinter> selectedPrinter;

    QString filename;
    ExportFormat format;
};
